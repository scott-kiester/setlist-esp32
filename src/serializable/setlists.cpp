#include "serializable/setlists.hpp"


namespace Serializable {

///////////////////////////////////////////////////////////////////////////////
// SetlistSong
///////////////////////////////////////////////////////////////////////////////
bool SetlistSong::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  try {
    name = obj["name"].as<const char*>();
    const char* strNotes = obj["notes"].as<const char*>();
    if (strNotes) {
      notes = strNotes;
    }
    
    return true;
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory copying setlist data");
    return false;
  }
}


///////////////////////////////////////////////////////////////////////////////
// Setlist
///////////////////////////////////////////////////////////////////////////////
bool Setlist::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  try {
    name = obj["name"].as<const char*>();

    ArduinoJson::JsonArray jsonSongs = obj["songs"].as<ArduinoJson::JsonArray>();
    for (ArduinoJson::JsonObject obj : jsonSongs) {
      SetlistSong *setlistSong = new SetlistSong();
      if (!setlistSong)  {
        logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Unable to allocate SetlistSong object during deserialization");
        return false;
      }

      if (setlistSong->DeserializeSelf(obj)) {
        setlistSongs.push_back(setlistSong);
      } else {
          delete setlistSong;
          logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Failed to deserialize setlistSong");
      }
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
