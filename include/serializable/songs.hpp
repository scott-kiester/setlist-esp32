#ifndef __SONGS_HPP___
#define __SONGS_HPP___

#include <list>
#include <string>

#include <ArduinoJson.hpp>

#include <serializable/serializable-object.hpp>

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
