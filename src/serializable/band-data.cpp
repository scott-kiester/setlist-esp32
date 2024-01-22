#include <ArduinoJson.hpp>

#include "serializable/band-data.hpp"


namespace Serializable {

///////////////////////////////////////////////////////////////////////////////
// BandData
///////////////////////////////////////////////////////////////////////////////
bool BandData::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  const char *nameStr = obj["name"];
  const char *pathStr = obj["path"];

  logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_VERBOSE, "Deserialize band data: {name: %s, path: %s}\n", nameStr, pathStr);

  try {
    name = nameStr;
    path = SDCARD_ROOT;
    path += pathStr;
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory copying band data");
    return false;
  }

  return true;
}




///////////////////////////////////////////////////////////////////////////////
// BandList
///////////////////////////////////////////////////////////////////////////////
BandList::~BandList() {
  for (Bands::iterator it = bands.begin(); it != bands.end(); ++it) {
    BandData *bandData = *it;
    bands.erase(it);
    if (bandData) {
      delete bandData;
    }
  }
}


bool BandList::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  ArduinoJson::JsonArray jsonBands = obj["bands"].as<ArduinoJson::JsonArray>();
  for (ArduinoJson::JsonObject jsonBand : jsonBands) {
    BandData *bandData = new BandData();
    if (!bandData) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "*****OUT OF MEMORY***** Unable to allocate BandData\n");
      return false;
    }

    if (!bandData->DeserializeSelf(jsonBand)) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "*** Fatal error deserializing BandData object\n");
      return false;
    }

    try {
      bands.push_back(bandData);
      logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_VERBOSE, "Added band to list: %s\n", bandData->GetName().c_str());
    } catch (std::bad_alloc&) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "***** OUT OF MEMORY ***** adding BandData object to array\n");
      return false;
    }
  }

  return true;
}

} // namespace Serializable
