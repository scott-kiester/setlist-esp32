#ifndef __SONGS_HPP___
#define __SONGS_HPP___

#include <list>
#include <string>

#include <ArduinoJson.hpp>

#include <serializable/serializable-object.hpp>


#define SONGS_FILE_PATH SDCARD_ROOT"/songs.json"

// Need to consider some changes here. Soon we won't have enough RAM
// to load all the songs.
//
// We'll let the server keep track of them in a single location, and 
// provide song info in its responses. The responses can then be stored
// with each set-list, which will need its own JSON file. That way, 
// we only need to keep one set's worth of songs in memory at any given 
// time, but we get the benefit of having a single song list at the server,
// so the information doesn't have to be entered multiple times.
#define SONGS_MAX_SIZE (8 * 1024)


namespace Serializable {
  /*  JSON format:
      {
        songs: [
          {
            "name": "Song name",
            "BPM": 120,
            "MP3": "the-mp3-file.mp3"
          },
          {
            "name": "Song 2",
            "BPM": 150,
            "MP3": "mp3-file.mp3"
          },
          {
            "name": "Another Song",
            "BPM": 95
          }
        ]
      }
  */

class Song : public SerializableObject {
public:
  Song(): bpm(0) {}
  virtual ~Song() {}

  const std::string& GetName() const { return name; };
  uint16_t GetBPM() const { return bpm; }
  const std::string& GetMp3File() const { return mp3File; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  std::string name;
  uint16_t bpm;
  std::string mp3File;
};

typedef std::list<Song*> Songs;

class SongList : public SerializableObject {
public:
  SongList() {}
  virtual ~SongList();
  
  const Songs& GetSongs() { return songs; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  Songs songs;
};

} // namespace Serializable

#endif
