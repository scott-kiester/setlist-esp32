#include <serializable/songs.hpp>

namespace Serializable {

///////////////////////////////////////////////////////////////////////////////
// Song
///////////////////////////////////////////////////////////////////////////////
bool Song::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_VERBOSE, "Song::DeserializeSelf\n");
  try {
    name = obj["name"].as<const char*>();
    bpm = obj["BPM"].as<unsigned short>();
    const char *mp3 = obj["MP3"];
    if (mp3) {
      mp3File = mp3;
    }

    logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_VERBOSE, "Song: %s, BPM: %d\n", name.c_str(), bpm);
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Out of memory copying song data");
    return false;
  }

  return true;
}



///////////////////////////////////////////////////////////////////////////////
// SongList
///////////////////////////////////////////////////////////////////////////////
SongList::~SongList() {
  for (Song *song : songs) {
    if (song) {
      delete song;
    }
  }
}


bool SongList::DeserializeSelf(const ArduinoJson::JsonObject& obj) {
  ArduinoJson::JsonArray jsonSongs = obj["songs"].as<ArduinoJson::JsonArray>();
  for (ArduinoJson::JsonObject jsonSong : jsonSongs) {
    Song *song = new Song();
    if (!song) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "*****OUT OF MEMORY***** Unable to allocate Song\n");
      return false;
    }

    if (!song->DeserializeSelf(jsonSong)) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "*** Fatal error deserializing Song object\n");
      return false;
    }

    try {
      songs.push_back(song);
      logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_VERBOSE, "Added song to list: %s\n", song->GetName().c_str());
    } catch (std::bad_alloc&) {
      logLn(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "***** OUT OF MEMORY ***** adding Song object to array\n");
      return false;
    }
  }

  return true;
}



} // namespace Serializable
