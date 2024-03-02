#include "log.hpp"
#include "serializable/config-data.hpp"


namespace Serializable {

///////////////////////////////////////////////////////////////////////////////
// AccessPoint
///////////////////////////////////////////////////////////////////////////////
bool AccessPoint::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  try {
    const char *strSsid = obj["ssid"].as<const char*>();
    if (!strSsid) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "SSID field is missing from AccessPoint");

      // This entry is no good, but others might be. Keep going.
      return true;
    }

    ssid = strSsid;

    const char *strPassword = obj["password"].as<const char*>();
    if (!strPassword) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Password field is missing from AccessPoint\n");
      return true;
    }

    password = strPassword;
    
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory reading AccessPoint configuration");
    return false;
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////
// WifiData
///////////////////////////////////////////////////////////////////////////////
WifiData::~WifiData() {
  for (AccessPoint *ap : apList) {
    if (ap) {
      delete ap;
    }
  }
}


bool WifiData::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  try {
    ArduinoJson::JsonArray jsonAps = obj["access-points"].as<ArduinoJson::JsonArray>();
    for (ArduinoJson::JsonObject obj : jsonAps) {
      AccessPoint *ap = new AccessPoint();
      if (!ap) {
        logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Unable to allocate AccessPoint object while reading config\n");
        return false;
      }

      if (ap->DeserializeSelf(obj)) {
        apList.push_back(ap);
      } else {
        logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Failed to deserialize AccessPoint\n");
        return false;
      }
    }
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory reading WiFi data");
    return false;
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////
// ConfigData
///////////////////////////////////////////////////////////////////////////////
bool ConfigData::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  try {
    ArduinoJson::JsonObject jsonWifi = obj["wifi"].as<ArduinoJson::JsonObject>();
    if (jsonWifi) {
      if (!wifiData.DeserializeSelf(jsonWifi)) {
        logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Failed to deserialize WifiData\n");
        return false;
      }
    } else {
      logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_WARN, "WiFi configuration not found\n");
    }
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory reading config data")
    return false;
  }

  return true;
}

} // namespace Serializable
