#ifndef __CONFIG_DATA_HPP___
#define __CONFIG_DATA_HPP___

#include <list>
#include <string>

#include <arduinoJson.hpp>

#include "serializable/serializable-object.hpp"

namespace Serializable {

#define DEFAULT_CONFIG_CHECK_INTERVAL_SECONDS 60
#define MAX_CONFIG_CHECK_INTERVAL_SECONDS (60 * 60)

/*
  JSON format:
  {
    wifi: {
      access-points: [
        {
          ssid: "the-ssid",
          password: "the-password"
        }
      ]
    }
  }

  access-points is an ordered list. We'll attempt to connect to each access point,
  starting at the top until we're successful.

  Yes, the password is in plain-text. I'll do something about that later. 
  (And... does it really matter in this case? It's just the WiFi password.)
*/

class AccessPoint : public SerializableObject {
public:
  AccessPoint() {}
  virtual ~AccessPoint() {}

  const std::string& GetSsid() const { return ssid; }
  const std::string& GetPassword() const { return password; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  std::string ssid;
  std::string password;
};


typedef std::list<AccessPoint*> ApList;


class WifiData : public SerializableObject {
public:
  WifiData() {}
  virtual ~WifiData();

  const ApList& GetApList() const { return apList; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private: 
  ApList apList;
};


class ConfigData : public SerializableObject {
public:
  ConfigData():
    configCheckIntervalSeconds(DEFAULT_CONFIG_CHECK_INTERVAL_SECONDS) {}

  virtual ~ConfigData() {}

  const WifiData& GetWifiData() const { return wifiData; }
  const std::string& GetDataUrl() const { return dataUrl; }
  uint32_t GetConfigCheckIntervalSeconds() const { return configCheckIntervalSeconds; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  WifiData wifiData;
  std::string dataUrl;
  uint32_t configCheckIntervalSeconds;
};

} // namespace Serializable

#endif
