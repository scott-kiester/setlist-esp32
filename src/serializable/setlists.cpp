#include "serializable/setlists.hpp"


namespace Serializable {

///////////////////////////////////////////////////////////////////////////////
// SetlistSong
///////////////////////////////////////////////////////////////////////////////
bool SetlistSong::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  try {
    const char *strName = obj["name"].as<const char*>();
    if (!strName) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Name field is missing from SetlistSong");

      // This entry is no good, but others might be. Keep going.
      return true;
    }

    name = strName;

    // Notes are optional
    const char *strNotes = obj["notes"].as<const char*>();
    if (strNotes) {
      notes = strNotes;
    }
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory copying setlist data");
    return false;
  }

  return true;
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
  for (Setlist *setList : sets) {
    if (setList) {
      delete setList;
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
