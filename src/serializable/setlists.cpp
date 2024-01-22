#include "serializable/setlists.hpp"


namespace Serializable {

///////////////////////////////////////////////////////////////////////////////
// Setlist
///////////////////////////////////////////////////////////////////////////////
bool Setlist::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  try {
    name = obj["name"].as<const char*>();

    ArduinoJson::JsonArray jsonSongs = obj["songs"].as<ArduinoJson::JsonArray>();
    for (ArduinoJson::JsonVariant v : jsonSongs) {
      setlistSongs.push_back(v.as<std::string>());
    }
    
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory copying setlist data");
    return false;
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////
// Setlists
///////////////////////////////////////////////////////////////////////////////
Setlists::~Setlists() {
  while (sets.size() > 0) {
    Setlist *setlist = sets.back();
    sets.pop_back();
    if (setlist) {
      delete setlist;
    }
  }
}


bool Setlists::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  try {
    ArduinoJson::JsonArray jsonSets = obj["sets"].as<ArduinoJson::JsonArray>();
    for (ArduinoJson::JsonObject obj : jsonSets) {
      Setlist *setlist = new Setlist();
      if (!setlist) {
        logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Unable to allocate Setlist object during deserialization");
        return false;
      }

      if (setlist->DeserializeSelf(obj)) {
        sets.push_back(setlist);
      } else {
        delete setlist;
        logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Failed to deserialize setlist");
      }
    }    
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory copying setlist data");
    return false;
  }

  return true;
}


} // namespace Serializable
