#include <dirent.h>

#include "config.hpp"
#include "log.hpp"
#include "network/remoteconfig.hpp"
#include "network/wifi.hpp"
#include "serializable/config-data.hpp"
#include "serializable/songs.hpp"
#include "storage/sdcard.hpp"
#include "tools/memorytools.hpp"


namespace Net {

#define CONFIG_MONITOR_THREAD_NAME "Cfg Monitor" // Max 16 bytes
#define CONFIG_DOWNLOAD_DIRECTORY SDCARD_ROOT"/config-download"

// I don't see anything larger than 2k in the examples. One even uses 512 bytes.
#define READ_CHUNK_SIZE 2048 

#define MAX_WAIT_FOR_CONNECTION_MS (30 * 1000)

///////////////////////////////////////////////////////////////////////////////
// local helper APIs
///////////////////////////////////////////////////////////////////////////////
bool clearDirectory(const char *dir, bool removeDir = false) {
  logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "Clearing directory %s\n", dir);
  DIR *dp = opendir(dir);
  if (!dp) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "Error opening directory %s\n", dir);
    return false;
  }

  std::string path(dir);
  struct dirent *ep = NULL;
  do {
    ep = readdir(dp);
    if (ep) {
      std::string fullName(dir);
      fullName += "/";
      fullName += ep->d_name;
      if (remove(fullName.c_str()) == 0) {
        logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "\tRemoved file: %s\n", fullName.c_str());
      } else {
        // Assume it's a directory
        clearDirectory(fullName.c_str(), true); 
      }
    }
  } while(ep);

  if (removeDir) {
    rmdir(dir);
  }

  closedir(dp);

  return true;
}



///////////////////////////////////////////////////////////////////////////////
// class HttpClient
///////////////////////////////////////////////////////////////////////////////

HttpClient::HttpClient():
  httpClient(NULL),
  readBuf(NULL),
  readBufLen(0) {}


HttpClient::~HttpClient() {
  if(httpClient) {
    esp_http_client_cleanup(httpClient);
  }

  if (readBuf) {
    delete readBuf;
  }
}


SetlistError HttpClient::Init(const std::string& _baseUrl) {
  SetlistError err = SL_ERR_OK;

  try {
    baseUrl = _baseUrl;

    delay(5000);

    esp_http_client_config_t clientConfig;
    memset(&clientConfig, 0, sizeof(esp_http_client_config_t));

    clientConfig.url = baseUrl.c_str();
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "Creating HTTP client handle. Base URL: %s\n", baseUrl.c_str());

    httpClient = esp_http_client_init(&clientConfig);
    if (!httpClient) {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Error creating HTTP client handle\n");
      err = SL_ERR_HTTP_CLIENT;
    }

  } catch (std::bad_alloc&) {
   err = SL_ERR_INSUFFICIENT_MEMORY;
  }

  return err;
}


std::string HttpClient::GetFilePath(const std::string& path) {
  std::string result;

  try {
    result = CONFIG_DOWNLOAD_DIRECTORY + path;
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "HttpClient: Out of memory building file path\n");
  }

  return result;
}


SetlistError HttpClient::GetToFile(const std::string& path) {
  SetlistError err = SL_ERR_OK;

  try {
    std::string fullUrl = baseUrl + path;
    std::string filePath = GetFilePath(path);


    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "HttpClient::GetToFile: full URL: %s\n", fullUrl.c_str());
    esp_err_t ret = esp_http_client_set_url(httpClient, fullUrl.c_str());
    if (ret != ESP_OK) {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Error from esp_http_client_set_url: %d. URL: %s\n", ret, fullUrl.c_str());
      return SL_ERR_HTTP_CLIENT;
    }

  // TODO: Call esp_http_client_set_header with If-None-Match or If-Modified-Since

    ret = esp_http_client_open(httpClient, 0);
    if (ret != ESP_OK) {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Error 0x%x (%s) on initial fetch of config. HTTP status: %d\n", 
        ret, esp_err_to_name(ret), esp_http_client_get_status_code(httpClient));
      return SL_ERR_HTTP_CLIENT;
    }

    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "RemoteConfig: Fetching headers\n");
    int totalLength = static_cast<int>(esp_http_client_fetch_headers(httpClient));
    if (totalLength < 0) {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "RemoteConfig: Error from esp_http_client_get_content_length: %d\n", static_cast<int>(totalLength));
      return SL_ERR_HTTP_CLIENT;
    }

    int64_t statusCode = esp_http_client_get_status_code(httpClient);
    if (statusCode != 200) {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "RemoteConfig: Unexpected HTTP status code: %d\n", statusCode);
      return SL_ERR_HTTP_SERVER;
    }

    if (totalLength > 0) {
      fs::File theFile;

      err = allocateReadBuf();
      if (!err) {
        // Create the file we'll be writing to.
        theFile = SDCard::GetFS()->open(filePath.c_str(), FILE_WRITE, true);
        if (!theFile) {
          err = SL_ERR_FILE_CREATE_FAILED;
          logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "HttpClient::GetToFile: Failed to create destination file\n");
        }
      }

      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "Http response is %d bytes long\n", totalLength);
      int totalRead = 0;
      while (!err && totalRead < totalLength) {
        int lengthRead = esp_http_client_read(httpClient, readBuf, readBufLen);
        if (lengthRead > 0) {
          totalRead += lengthRead;
          size_t writeResult = theFile.write(reinterpret_cast<uint8_t*>(readBuf), lengthRead);
          if (writeResult > 0) {
            logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "HttpClient: Read %d bytes from server and wrote them to file\n", lengthRead);
          } else {
            err = SL_ERR_FILE_WRITE;
            logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "HttpClient: Error writing HTTP response to file: %s\n", path.c_str());
          }
        } else if (totalRead == 0) {
          err = SL_ERR_READ_HTTP_RESPONSE_FAILED;
          logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "HttpClient: Unable to read response from %s\n", fullUrl.c_str());
        }
      }

      if (!err) {
        logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "HttpClient: Successfully wrote %d bytes to file %s\n", totalRead, path.c_str());
      }

      if (theFile) {
        theFile.flush();
        logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "HttpClient: Flushed file: %s\n", path.c_str());
        theFile.close();
        logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "HttpClient: Closed file: %s\n", path.c_str());
      }

    } else {
      err = SL_ERR_HTTP_CLIENT;
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Invalid content length on inital fetch: %d\n", totalLength);
    }

  } catch (std::bad_alloc&) {
    err = SL_ERR_INSUFFICIENT_MEMORY;
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "std::bad_alloc exception in HttpClient::GetToFile\n");
  }

  logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "Leaving HttpClient...");

  return err;
}


SetlistError HttpClient::allocateReadBuf() {
  if (readBuf) {
    return SL_ERR_OK;
  }

  readBuf = new char[READ_CHUNK_SIZE];
  if (!readBuf) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Out of memory allocating read buffer in HttpClient::GetToFile\n");
    return SL_ERR_INSUFFICIENT_MEMORY;
  }

  readBufLen = READ_CHUNK_SIZE;
  return SL_ERR_OK;
}



///////////////////////////////////////////////////////////////////////////////
// class RemoteConfig
///////////////////////////////////////////////////////////////////////////////

RemoteConfig::RemoteConfig():
  configEvent(NULL),
  haveInitialConfig(false) {}


RemoteConfig::~RemoteConfig() {
  if(configEvent) {
    vEventGroupDelete(configEvent);
  }
}


RemoteConfig* RemoteConfig::GetRemoteConfig() {
  static RemoteConfig remoteConfig;
  return &remoteConfig;
}


SetlistError RemoteConfig::getFile(HttpClient& httpClient, const std::string& partialFileUrl) {
  SetlistError err = httpClient.GetToFile(partialFileUrl);
  if (err) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Downloading new config: Failed to fetch %s: %d(%s)\n", 
      partialFileUrl.c_str(), err, SetlistErrorToString(err));
  }
 
  return err;
}


SetlistError RemoteConfig::downloadAndApplyNewConfig(const std::string& dataUrl) {
  // Establish an HTTP connection to the remote site and download stuff to a temp
  // directory.
  //
  // Start by fetching config.json, band-data.json, and songs.json asynchronously. For
  // each band in band-data.json, asynchronously fetch the setlists.
  // 
  // Once everything has been downloaded to a temp directory, validate the config files.
  // If they're valid, then apply them.
  //
  // TODO: When applying config, need to determine how things will be updated. If the
  // BPM for the current song changes, then we probably don't want to update that until
  // the user does something, such as moves to the next song. However, other things like
  // the notes, we want to have downloaded in real time so that if one person changes
  // them, then everyone else sees the change quickly.
  //
  // Future: Consider keeping a socket open to the server and having the server notify
  // us in real-time if there's a change to fetch. (How feasible is that? How expensive
  // (in terms of AWS $$) is that?)

  // Doing this asynchronously is ideal, but none of this is performance-critical. I'm going
  // to do it the easy way for now, so I can have something working. (My inner engineer is
  // absolutely screaming at me right now.)

  // I've chosen to use the ESP HTTP client here - rather than Arduino's HTTPClient - because
  // it can natively support async connections, which means I won't have to manage a thread
  // pool to make it work effectively.

  SetlistError err = SL_ERR_OK;

  // Clear out our temp directory before we start
  if (!clearDirectory(CONFIG_DOWNLOAD_DIRECTORY)) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Failed to clear temp directory for RemoteConfig. Dir: %s\n", CONFIG_DOWNLOAD_DIRECTORY);
    return SL_ERR_FILE_WRITE; // TODO: Need a better error code here
  }

  // We can't do jack if we're not connected!
  if (!Net::WaitForConnection(MAX_WAIT_FOR_CONNECTION_MS)) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "RemoteConfig: Timed out waiting for WiFi connection\n");
    return SL_ERR_NO_NETWORK;
  }

  logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "RemoteConfig: Network connection present\n");
  HttpClient httpClient;
  err = httpClient.Init(dataUrl);
  if (err) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Error initializing HTTP client: %d\n", err);
    return err;
  }

  // Grab config.hpp first. 
  if ((err = getFile(httpClient, "/config.json")) != SL_ERR_OK) {
    return err;
  }

  std::shared_ptr<Config> newConfig = Config::GetConfigFrom(httpClient.GetFilePath("/config.json"));
  if (!newConfig) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Unable to process config.json from %s.\n", dataUrl.c_str());
    return SL_ERR_BAD_CONFIG;
  }

  if ((err = getFile(httpClient, "/band-data.json")) != SL_ERR_OK) {
    return err;
  }

  Serializable::BandList bandList;
  if (!bandList.DeserializeObject(httpClient.GetFilePath("/band-data.json").c_str(), BAND_DATA_FILE_MAXSIZE) || !bandList.IsValid()) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Unable to process band-data.json from %s.\n", dataUrl.c_str());
    return SL_ERR_BAD_CONFIG;
  }

  if ((err = getFile(httpClient, "/songs.json")) != SL_ERR_OK) {
    return err;
  }

  logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "About to deserialize songs.json\n");
  Serializable::SongList songList;
  if (!songList.DeserializeObject(httpClient.GetFilePath("/songs.json").c_str(), SONGS_MAX_SIZE) || !songList.IsValid()) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Unable to process songs.json from %s.\n", dataUrl.c_str());
    return SL_ERR_BAD_CONFIG;
  }

    
  logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "Remote config loaded from %s verified successfully\n", dataUrl.c_str());

/*
  fs::File config = SDCard::GetFS()->open((std::string(CONFIG_DOWNLOAD_DIRECTORY) + "/config.json").c_str());
  if (config) {
    char *contents = new char[config.size() + 1];
    size_t bytesRead = config.readBytes(contents, config.size());
    if (bytesRead > 0) {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "NULL-terminating 'contents' array\n");
      contents[config.size()] = '\0';
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "config.json: \n");
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, contents);
      haveInitialConfig = true;

      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "Leaving RemoteConfig::downloadAndApplyNewConfig()\n");
    } else {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Failed to read config.json\n");
    }

  } else {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Failed to read back config.json\n");
  }
*/
  return err;
}


SetlistError RemoteConfig::DownloadAndApplyNewConfig() {
  SetlistError err = SL_ERR_OK;

  std::shared_ptr<const Config> config = Config::GetConfig();
  if (config) {
    const Serializable::ConfigData *configData = config->GetConfigData();
    if (configData) {
      const std::string &dataUrl = configData->GetDataUrl();
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "Calling..........\n");
      err = downloadAndApplyNewConfig(dataUrl);
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "RemoteConfig::downloadAndApplyNewConfig returns: %d (%s)\n", 
        err, SetlistErrorToString(err));
    } else {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "RemoteConfig::DownloadAndApplyRemoteConfig: config->GetConfigData() returned NULL\n");
      err = SL_ERR_BAD_CONFIG;
    }
  } else {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "RemoteConfig::DownloadAndApplyNewConfig: Unable to obtain config.");
    err = SL_ERR_BAD_CONFIG;
  }

  return err;
}


void RemoteConfig::configMonitorThread() {
  logLn(LOG_COMP_RMT_CONFIG, LOG_SEV_INFO, "Config Monitor Thread started");

  while(true) {
    if (!haveInitialConfig) {
      try {
        SetlistError err = DownloadAndApplyNewConfig();
        logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "configMonitorThread: DownloadAndApplyNewConfig returns %d (%s)\n",
          err, SetlistErrorToString(err));
      } catch (std::exception& e) {
        logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_VERBOSE, "Exception from DownloadAndApplyNewConfig()\n");
        logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Exception from DownloadAndApplyNewConfig(): %s\n", e.what());
      }
    }

    delay(10 * 1000);

    //delay(10000);
    /*
    uint32_t configCheckIntervalSeconds = DEFAULT_CONFIG_CHECK_INTERVAL_SECONDS;

    const Serializable::ConfigData* configData = NULL;
    const Config* config = Config::GetConfig();
    if (config) {
      configData = config->GetConfigData();
      configCheckIntervalSeconds = configData->GetConfigCheckIntervalSeconds();
    }

    const std::string& dataUrl = configData ? configData->GetDataUrl() : std::string();
    if (dataUrl.length() != 0) {
      SetlistError err = downloadAndApplyNewConfig(dataUrl);
    } else {
      logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_WARN, "RemoteConfig::configMonitorThread: dataUrl is NULL. Can't refresh config.\n");
    }

    // NEED TO WAIT FOR A SIGNAL HERE - DELAY FOR configCheckIntervalSeconds amount of time.
    */
  }
}


void RemoteConfig::configMonitorThreadCallback(void *param) {
  reinterpret_cast<RemoteConfig*>(param)->configMonitorThread();
}


bool RemoteConfig::init() {
  // This filesystem implementation is weird. Lots of the POSIX calls don't work correctly. Frustrating.
  if (!clearDirectory(CONFIG_DOWNLOAD_DIRECTORY)) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Failed to clear temp directory for RemoteConfig. Dir: %s\n", CONFIG_DOWNLOAD_DIRECTORY);
    return false;
  }

  // TODO: Is this actually needed anymore?
  configEvent = xEventGroupCreate();
  if (!configEvent) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Failed to create configEvent\n");
    return false;
  }

  // Spin off a thread that will periodically check for updated configs.
  RemoteConfig* rc = GetRemoteConfig();
  BaseType_t ret = xTaskCreate(&configMonitorThreadCallback, CONFIG_MONITOR_THREAD_NAME, 1024 * 4, rc, 5, NULL);
  if (ret == pdPASS) {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_INFO, "Remote Config Initialied\n");
  } else {
    logPrintf(LOG_COMP_RMT_CONFIG, LOG_SEV_ERROR, "Failed to create configMonitorThread: 0x%x\n", ret);
  }

  return true;
}


bool RemoteConfig::Init() {
  return GetRemoteConfig()->init();
}

} // namespace Net
