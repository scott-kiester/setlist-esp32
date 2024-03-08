#ifndef __REMOTE_CONFIG_HPP___
#define __REMOTE_CONFIG_HPP___

#include <esp_http_client.h>

#include "setlisterror.hpp"

namespace Net {

class HttpClient {
public:
  HttpClient();
  virtual ~HttpClient();

  SetlistError Init(const std::string& _baseUrl);
  SetlistError GetToFile(const std::string& path);

  std::string GetFilePath(const std::string& path);

private:
  SetlistError allocateReadBuf();

  esp_http_client_handle_t httpClient;
  std::string baseUrl;

  char *readBuf;
  uint32_t readBufLen;
};

class RemoteConfig {
public:
  RemoteConfig();
  virtual ~RemoteConfig();

  SetlistError DownloadAndApplyNewConfig();

  static bool Init();
  static RemoteConfig* GetRemoteConfig();

private:
  bool init();
  static void configMonitorThreadCallback(void *param);
  void configMonitorThread();

  SetlistError downloadAndApplyNewConfig(const std::string &dataUrl);
  SetlistError getFile(HttpClient& httpClient, const std::string& fullFileUrl);

  EventGroupHandle_t configEvent;
  bool haveInitialConfig;
};

} // namespace Net
#endif
