#include <set>

#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include "config.hpp"
#include "debugchecks.hpp"
#include "log.hpp"
#include "network/wifi.hpp"

#define DEFAULT_SCAN_LIST_SIZE 50
#define WIFI_CONNECTION_ESTABLISH_WAIT_SECONDS 15
#define WIFI_CONNECTION_RETRY_SECONDS 5
#define WIFI_CONNECTION_RETRY_MAX_SECONDS (60 * 5)

#define WIFI_START_FAILURE_SECONDS 5

#define WIFI_THREAD_NAME "WiFi Manager" // Max 16 bytes

// Turns out there's already a WIFI_EVENT_CONNECTED somewhere. Go figure.
#define LOCAL_WIFI_EVENT_ALL          0xffffffff
#define LOCAL_WIFI_EVENT_STA_START    0x00000001
#define LOCAL_WIFI_EVENT_FAILED       0x00000002
#define LOCAL_WIFI_EVENT_CONNECTED    0x00000004
#define LOCAL_WIFI_EVENT_DISCONNECTED 0x00000008
#define LOCAL_WIFI_EVENT_GOT_IP       0x00000010

namespace Net {

///////////////////////////////////////////////////////////////////////////////
// Private helper routines
///////////////////////////////////////////////////////////////////////////////

static void printAuthMode(int authmode)
{
  switch (authmode) {
  case WIFI_AUTH_OPEN:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_OPEN\n");
    break;
  case WIFI_AUTH_WEP:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_WEP\n");
    break;
  case WIFI_AUTH_WPA_PSK:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_WPA_PSK\n");
    break;
  case WIFI_AUTH_WPA2_PSK:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_WPA2_PSK\n");
    break;
  case WIFI_AUTH_WPA_WPA2_PSK:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_WPA_WPA2_PSK\n");
    break;
  case WIFI_AUTH_WPA2_ENTERPRISE:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_WPA2_ENTERPRISE\n");
    break;
  case WIFI_AUTH_WPA3_PSK:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_WPA3_PSK\n");
    break;
  case WIFI_AUTH_WPA2_WPA3_PSK:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_WPA2_WPA3_PSK\n");
    break;
  default:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Authmode \t\tWIFI_AUTH_UNKNOWN\n");
    break;
  }
}

static void printCipherType(int pairwise_cipher, int group_cipher)
{
  switch (pairwise_cipher) {
  case WIFI_CIPHER_TYPE_NONE:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE\n");
    break;
  case WIFI_CIPHER_TYPE_WEP40:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40\n");
    break;
  case WIFI_CIPHER_TYPE_WEP104:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104\n");
    break;
  case WIFI_CIPHER_TYPE_TKIP:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP\n");
    break;
  case WIFI_CIPHER_TYPE_CCMP:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP\n");
    break;
  case WIFI_CIPHER_TYPE_TKIP_CCMP:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP\n");
    break;
  default:
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN\n");
    break;
  }

  switch (group_cipher) {
  case WIFI_CIPHER_TYPE_NONE:
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Group Cipher \t\tWIFI_CIPHER_TYPE_NONE\n");
      break;
  case WIFI_CIPHER_TYPE_WEP40:
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Group Cipher \t\tWIFI_CIPHER_TYPE_WEP40\n");
      break;
  case WIFI_CIPHER_TYPE_WEP104:
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Group Cipher \t\tWIFI_CIPHER_TYPE_WEP104\n");
      break;
  case WIFI_CIPHER_TYPE_TKIP:
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Group Cipher \t\tWIFI_CIPHER_TYPE_TKIP\n");
      break;
  case WIFI_CIPHER_TYPE_CCMP:
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Group Cipher \t\tWIFI_CIPHER_TYPE_CCMP\n");
      break;
  case WIFI_CIPHER_TYPE_TKIP_CCMP:
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Group Cipher \t\tWIFI_CIPHER_TYPE_TKIP_CCMP\n");
      break;
  default:
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Group Cipher \t\tWIFI_CIPHER_TYPE_UNKNOWN\n");
      break;
  }
}



///////////////////////////////////////////////////////////////////////////////
// WiFi
///////////////////////////////////////////////////////////////////////////////
class WiFi {
private:
  WiFi():
    wifiEvent(NULL),
    connected(false),
    haveIp(false) {}

public:
  virtual ~WiFi();

  bool Init();
  bool WaitForWiFiStart();
  bool WaitForConnection(uint32_t maxWaitMs);

  static WiFi* GetWiFi();

private:
    bool connect();
    bool connectToAp(const std::string& ssid, const std::string& password);

    void wifiThread();
    static void wifiThreadCallback(void *param);

    void eventHandler(esp_event_base_t eventBase, int32_t eventId, void *eventData);
    static void eventHandlerCallback(void *param, esp_event_base_t eventBase, int32_t eventId, void *eventData);


    EventGroupHandle_t wifiEvent;

    bool connected;
    bool haveIp;
    std::string apSsid;
    std::string apPassword;
};



///////////////////////////////////////////////////////////////////////////////
// WiFi implementation
///////////////////////////////////////////////////////////////////////////////

WiFi::~WiFi() {
  if (wifiEvent) {
    vEventGroupDelete(wifiEvent);
  }
}


WiFi* WiFi::GetWiFi() {
  static WiFi wiFi;
  return &wiFi;
}


bool WiFi::WaitForWiFiStart() {
  EventBits_t theBits = xEventGroupWaitBits(wifiEvent, LOCAL_WIFI_EVENT_STA_START, pdTRUE, pdFALSE, portMAX_DELAY);
  if (theBits & LOCAL_WIFI_EVENT_STA_START) {
    return true;
  }

  return false;
}


bool WiFi::WaitForConnection(uint32_t maxWaitMs) {
  if (connected && haveIp) {
    return true;
  } 

  logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Waiting for network. Connected: %s, haveIp: %s\n",
    connected ? "true" : "false",
    haveIp ? "true" : "false")
  EventBits_t theBits = xEventGroupWaitBits(wifiEvent, LOCAL_WIFI_EVENT_GOT_IP, pdTRUE, pdFALSE, maxWaitMs);
  if (theBits & LOCAL_WIFI_EVENT_GOT_IP) {
    return true;
  }

  return false;
}


// This should only be called from the Wifi Thread
bool WiFi::connect() {
  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "WiFi: Connecting...\n");

  if (connected) {
    // Huh. We shouldn't be here if connected == true.
    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "WiFi::connect(): connected == true\n");

    // Are we REALLY connected?
    wifi_ap_record_t ap;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap);
    if (ret == ESP_OK) {
      // Yep, we're connected. There's probably a bug.
      if (apSsid.length() == 0 || apSsid.compare(reinterpret_cast<char*>(ap.ssid)) == 0) {
        logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "BUG: WiFi::connect() called while already connected, but there's no apSsid.\n");
      } else {
        logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "BUG: WiFi::connect() called while connect == true. We're actually connected to %s, but expected %s\n",
          ap.ssid, apSsid.c_str());

        // I'm not going to correct apSsid / apPassword here, because we shouldn't be here to begin with.
        // How did they get out of sync? If we're here, then that problem needs to be understood and fixed.
      }
      
      return true;

    } else if (ret != ESP_ERR_WIFI_NOT_CONNECT) {
      // Uh oh. Something is not happy.
      logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "WiFi::connect(): Got error %d from esp_wifi_sta_get_ap_info\n", ret);
      // Proceed anyway. Maybe we can reconnect.

    } else {
      // We really are disconnected. Why was 'connected' true?
      logPrintf(LOG_COMP_NET, LOG_SEV_WARN, "WiFi::connect() called with connect == true, but we're actually disconnected. Bug?\n");
    }

    connected = haveIp = false;
  }

  try {
    // Keep track of the SSIDs we've failed to connect to so that we don't try again until next time we're called.
    std::set<std::string> failedSsids;

    // If we were previously connected to an AP, then try to reconnect to it. 
    if (apSsid.length() > 0) {
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Attempting to re-connect to last used AP. SSID: %s\n", apSsid.c_str());
      if (connectToAp(apSsid, apPassword)) {
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Successfully re-connected to previously used AP: %s\n", apSsid);
        return true;
      }

      failedSsids.insert(apSsid);
    }

    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "WiFi: Fetching config...\n");
    // List the networks and see if we're configured for any of them.
    std::shared_ptr<const Config> config = Config::GetConfig();
    if (!config) {
      logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "WiFi::connect(): Config is NULL!\n");
      return false;
    }

    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "WiFi: Fetching AP list...\n");
    const Serializable::ApList& apList = config->GetConfigData()->GetWifiData().GetApList();

    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Listing SSIDs...\n");
    SsidList listedSsids;
    if (ListNetworks(&listedSsids)) {
      for (const Serializable::AccessPoint* ap : apList) {
        for (const std::string& listedSsid : listedSsids) {
          if (listedSsid.compare(ap->GetSsid()) == 0) {
            // Found one!
            if (failedSsids.find(listedSsid) != failedSsids.end()) {
              logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Skipping SSID %s because previous attempt failed\n", listedSsid);
              continue;
            }

            logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "SSID %s found. Attempting to connect...\n", listedSsid.c_str());
            if (connectToAp(ap->GetSsid(), ap->GetPassword())) {
              // SUCCESS!
              connected = true;
              logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Connected to SSID %s\n", listedSsid.c_str());
              return true;
            } else {
              logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Connetion to SSID %s failed\n", listedSsid.c_str());
              failedSsids.insert(listedSsid);
            }
          }
        }
      }
    } else {
      logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "ListNetworks failed in Wifi::connect()\n");
      // Don't give up! Maybe the SSID isn't advertised.
    }

    // Now just run down the list in our configuration. Perhaps the network we want isn't broadcasting its SSID.
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Attempting connection to potentially hidden SSIDs that are in our configuration\n");

    for (const Serializable::AccessPoint* ap : apList) {
      if (failedSsids.find(ap->GetSsid()) != failedSsids.end()) {
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Skipping SSID %s because previous attempt failed\n", ap->GetSsid().c_str());
        continue;
      }

      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Attempting connection SSID %s\n", ap->GetSsid().c_str());
      if (connectToAp(ap->GetSsid(), ap->GetPassword())) {
        connected = true;
        apSsid = ap->GetSsid();
        apPassword = ap->GetPassword();
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Successfully connected to SSID %s\n", ap->GetSsid().c_str());
        return true;
      } else {
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Connetion to SSID %s failed\n", ap->GetSsid().c_str());
      }
    }
  } catch(std::bad_alloc&) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Out of memory tracking failed networks in WiFi::connect()\n");
  }

  // If we get here, then we were unable to connect to anything.
  return false;
}


bool WiFi::connectToAp(const std::string& ssid, const std::string& password) {
  wifi_config_t wifiConfig;
  memset(&wifiConfig, 0, sizeof(wifi_config_t));

  // Leave space for the NULL
  if (ssid.length() > sizeof(wifiConfig.sta.ssid) - 1) {
    logPrintf(LOG_COMP_NET, LOG_SEV_WARN, "SSID %s is too long. Max is %d bytes.\n", ssid.c_str(), sizeof(wifiConfig.sta.ssid));
    return false;
  }

  strcpy(reinterpret_cast<char*>(wifiConfig.sta.ssid), ssid.c_str());

  if (password.length() > sizeof(wifiConfig.sta.password) - 1) {
    logPrintf(LOG_COMP_NET, LOG_SEV_WARN, "Password for SSID %s is too long. Max is %d bytes.\n", 
      wifiConfig.sta.ssid, sizeof(wifiConfig.sta.password));

    return false;
  }

  strcpy(reinterpret_cast<char*>(wifiConfig.sta.password), password.c_str());

  // Minimum tolerable security configration. Until I create an HTTPS-only version of troutlemonade.com,
  // encryption will be a requirement.
  wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA_PSK; 

  // Apparently this is the old key exchange mechanism for SAE. To do the new method - "hash to element",
  // we need a password. I'm pretty sure I'll need to add that for WPA3 support - it will have to be done at
  // some point.
  wifiConfig.sta.sae_pwe_h2e = WPA3_SAE_PWE_HUNT_AND_PECK; 

  // Is it necessary to disconnect here? We wouldn't be here if we thought we were connected. What does the 
  // implementation do if the connnection gets into a bad state, and then we re-connect to the same AP? Disconnecting
  // would guarantee that any existing connections would be torn down, and that's not what we want.
  //esp_wifi_disconnect();

  logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Attempting connection to SSID %s\n", ssid.c_str());
  esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error %d from esp_wifi_set_config. SSID: %s\n", ret, ssid.c_str());
    return false;
  }

  ret = esp_wifi_connect();
  if (ret == ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Success returned from esp_wifi_connect\n");
  } else {
    // Some errors here are expected, as not all of our configured APs will be in range all the time.
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Error from esp_wifi_connect: %d, SSID: %s\n", ret, ssid.c_str());
    return false;
  }

  // Wait until our callback is invoked.
  EventBits_t event = xEventGroupWaitBits(wifiEvent, LOCAL_WIFI_EVENT_GOT_IP | LOCAL_WIFI_EVENT_FAILED | LOCAL_WIFI_EVENT_DISCONNECTED, 
    pdTRUE, pdFALSE, WIFI_CONNECTION_ESTABLISH_WAIT_SECONDS * 1000);

  if (event & LOCAL_WIFI_EVENT_GOT_IP) {
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Connection successful! SSID: %s\n", ssid.c_str());
  } else {
    // Failure or timeout.
    if (event & LOCAL_WIFI_EVENT_FAILED || event & LOCAL_WIFI_EVENT_DISCONNECTED) {
      logPrintf(LOG_COMP_NET, LOG_SEV_WARN, "Failed to connect to SSID %s. Event: 0x%x\n", ssid.c_str(), event);
    } else {
      logPrintf(LOG_COMP_NET, LOG_SEV_WARN, "Timed out connecting to SSID %s. Event bits: 0x%x\n", ssid.c_str(), event);
    }

    return false;
  }

  return true;
}


void WiFi::wifiThread() {
  // Start up WiFi and maintain the connection with our AP
  esp_err_t ret = 0;
  do {
    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Calling esp_wifi_start()\n");
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
      logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error from esp_wifi_start: %d\n", ret);
      // Whelp... Keep trying, I guess. Nothing to lose at this point.
      delay(WIFI_START_FAILURE_SECONDS * 1000);
    }
  } while(ret != ESP_OK);

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Entering connected loop\n");
  uint32_t retryDelay = WIFI_CONNECTION_RETRY_SECONDS;
  while (true) {
    if (!connected) {
      logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "wifiThread: Not connected. Attempting to connect...\n");
      if (!connect()) {
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "wifiThread: Connection attempt failed. Waiting %d seconds before trying again.\n", retryDelay); 
        delay(retryDelay * 1000);

        // Wait twice as long next time, unless that exceeds the max.
        retryDelay *= 2;
        if (retryDelay > WIFI_CONNECTION_RETRY_MAX_SECONDS) {
          retryDelay = WIFI_CONNECTION_RETRY_MAX_SECONDS;
        }
        
        continue;
      }
    }

    if (!connected) {
      // WTF?
      logPrintf(LOG_COMP_NET, LOG_SEV_WARN, "wifiThread: Connection appeared to be successful - just now - but we're no longer connected!\n");
      continue;    
    }

    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "wifiThread: Waiting for event...\n");
    retryDelay = WIFI_CONNECTION_RETRY_SECONDS;
    EventBits_t bits = xEventGroupWaitBits(wifiEvent, LOCAL_WIFI_EVENT_DISCONNECTED, pdTRUE, pdFALSE, portMAX_DELAY);
    // It doesn't matter why we stopped waiting. Either way we need to make sure we're connected and
    // attempt to reconnect if necessary.
    logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "wifiThread awake with event bits: 0x%x\n", bits);
  }
}


void WiFi::wifiThreadCallback(void *param) {
  reinterpret_cast<WiFi*>(param)->wifiThread();
}


void WiFi::eventHandler(esp_event_base_t eventBase, int32_t eventId, void *eventData) {
  EventBits_t setEvents = 0;
  if (eventBase == WIFI_EVENT) {
    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Wifi::eventHandler: Got WiFi event 0x%x\n", eventId);

    switch(eventId) {
      case WIFI_EVENT_STA_START:
        setEvents |= LOCAL_WIFI_EVENT_STA_START;
        break;

      case WIFI_EVENT_STA_CONNECTED:
        setEvents |= LOCAL_WIFI_EVENT_CONNECTED;
        break;

      case WIFI_EVENT_STA_DISCONNECTED:
        setEvents |= LOCAL_WIFI_EVENT_DISCONNECTED;
        connected = haveIp = false;
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "WiFi received disconnect event\n");
        break;
      
      default:
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Received WiFi event %d. Event not processed\n", eventId);
    }
  } else if (eventBase == IP_EVENT) {
    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Wifi::eventHandler: Got IP event 0x%x\n", eventId);

    switch(eventId) {
      case IP_EVENT_STA_GOT_IP:
        haveIp = true;
        setEvents |= LOCAL_WIFI_EVENT_GOT_IP;
        if (IS_LOG_ENABLED(LOG_COMP_NET, LOG_SEV_INFO)) {
          // For future reference, to get the IP elsewhere in the code:
          //  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
          ip_event_got_ip_t *ipEvent = reinterpret_cast<ip_event_got_ip_t*>(eventData);

          char ipBuf[IP4ADDR_STRLEN_MAX];
          esp_ip4addr_ntoa(&ipEvent->ip_info.ip, ipBuf, IP4ADDR_STRLEN_MAX);

          logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Got IPv4 address: %s\n", ipBuf);
        }
        break;

      case IP_EVENT_GOT_IP6:
        haveIp = true;
        setEvents |= LOCAL_WIFI_EVENT_GOT_IP;
        if (IS_LOG_ENABLED(LOG_COMP_NET, LOG_SEV_INFO)) {
          ip_event_got_ip6_t *ipEvent = reinterpret_cast<ip_event_got_ip6_t*>(eventData);
        
          // Whelp, Espressif doesn't seem to provide a thread-safe ip-to-string conversion
          // function for IPv6. I guess I'll use the unsafe one from their examples. (I'm not 
          // going to roll my own for a trace message.)
          logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Got IPv6 address: %s\n", IPV62STR(ipEvent->ip6_info.ip));
        }
        break;

      case IP_EVENT_STA_LOST_IP:
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Lost Ip address\n");
        connected = haveIp = false;
        setEvents |= LOCAL_WIFI_EVENT_DISCONNECTED;
        break;


      default:
        logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Received IP event %d. Event not processed\n", eventId);
    }
  } else {
    // Shouldn't get here
    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Wifi::eventHandler: Ignoring non-WiFi event\n");
  }

  if (setEvents != 0) {
    logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Wifi::eventHandler: Setting event bits: 0x%x\n", setEvents);
    xEventGroupSetBits(wifiEvent, setEvents);
  }
}


void WiFi::eventHandlerCallback(void *param, esp_event_base_t eventBase, int32_t eventId, void *eventData) {
  reinterpret_cast<WiFi*>(param)->eventHandler(eventBase, eventId, eventData);
}


bool WiFi::Init() {
  // Initialize NVS. Taken from the example.
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ret = nvs_flash_erase();
      if (ret != ESP_OK) {
        logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error from nvs_flash_erase: %d\n", ret);
      }

      ret = nvs_flash_init();
      if (ret != ESP_OK) {
        logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error from nvs_flash_init: %d\n", ret);
      }
  }

  if (ret != ESP_OK) {
    return false;
  }

  wifiEvent = xEventGroupCreate();
  if (!wifiEvent) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Failed to create wifiEvent\n");
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Calling esp_netif_init()\n");

  ret = esp_netif_init();
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error form esp_netif_init: %d\n", ret);
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Calling esp_event_loop_create_default()\n");
  ret = esp_event_loop_create_default();
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error from esp_event_loop_create_default: %d\n", ret);
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Calling esp_netif_create_default_wifi_sta()\n");
  esp_netif_t *staNetif = esp_netif_create_default_wifi_sta();
  DC_ASSERT(staNetif != NULL);
  if (!staNetif) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "NULL value returned from esp_netif_create_default_wifi_sta\n");
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Calling esp_wifi_init()\n");
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ret = esp_wifi_init(&cfg);
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error from esp_wifi_init: %d\n", ret);
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Calling esp_event_handler_instance_register() for WiFi\n");
  ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandlerCallback, this, NULL);
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error registering WiFi event handler: %d\n", ret);
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Calling esp_event_handler_instance_register() for IP\n");
  ret = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &eventHandlerCallback, this, NULL);
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error registering IP event handler: %d\n", ret);
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Calling esp_wifi_set_mode()\n");
  ret = esp_wifi_set_mode(WIFI_MODE_STA);
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error from esp_wifi_set_mode: %d\n", ret);
    return false;
  }

  // We'll let the thread handle the connection.
  xTaskCreate(&wifiThreadCallback, WIFI_THREAD_NAME, 1024 * 6, this, 5, NULL);

  logPrintf(LOG_COMP_NET, LOG_SEV_VERBOSE, "Waiting for WiFi start\n");

  if (!WaitForWiFiStart()) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "WiFi failed to start\n");
    return false;
  }

  return true;
}



///////////////////////////////////////////////////////////////////////////////
// Externally visible routines
///////////////////////////////////////////////////////////////////////////////

bool WaitForConnection(uint32_t maxWaitMs) {
  return WiFi::GetWiFi()->WaitForConnection(maxWaitMs);
}


bool ListNetworks(SsidList *ssidList) {
  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  wifi_ap_record_t apInfo[DEFAULT_SCAN_LIST_SIZE];
  uint16_t apCount = 0;
  memset(apInfo, 0, sizeof(apInfo));

  logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "\nListing networks...\n");

  esp_wifi_scan_start(NULL, true);
  esp_err_t ret = esp_wifi_scan_get_ap_records(&number, apInfo);
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error from esp_wifi_scan_get_ap_records: %d\n", ret);
    return false;
  }

  ret = esp_wifi_scan_get_ap_num(&apCount);
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Error from esp_wifi_scan_get_ap_num: %d\n", ret);
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Total APs scanned = %d\n", apCount);
  for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < apCount); i++) {
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "SSID \t\t\t%s\n", apInfo[i].ssid);
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "RSSI \t\t\t%d\n", apInfo[i].rssi);
      printAuthMode(apInfo[i].authmode);
      if (apInfo[i].authmode != WIFI_AUTH_WEP) {
          printCipherType(apInfo[i].pairwise_cipher, apInfo[i].group_cipher);
      }
      logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Channel \t\t%d\n\n", apInfo[i].primary);

      try {
        if (ssidList) {
          ssidList->push_back(reinterpret_cast<char*>(apInfo[i].ssid));
        }
      } catch (std::bad_alloc&) {
        logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "Out of memory allocating list entry for ssidList in Net::ListNetworks.\n");
        return false;
      }
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Done listing networks\n\n");

  return true;
}


bool Init() {
  logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "Initializing WiFi...\n");

  if (!WiFi::GetWiFi()->Init()) {
    logPrintf(LOG_COMP_NET, LOG_SEV_ERROR, "WiFi initialization failed\n");
    return false;
  }

  logPrintf(LOG_COMP_NET, LOG_SEV_INFO, "WiFi initialized!\n");

  return true;
}


} // namespace Net