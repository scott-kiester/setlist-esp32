#ifndef __SETS_HPP___
#define __SETS_HPP___

#include <list>
#include <string>

#include <ArduinoJson.hpp>

#include <serializable/serializable-object.hpp>

namespace Serializable {
  /*  JSON format:
      {
        "sets": [
          {
            "name": "Scera",
            songs: [
              {
                "name": "Song name"
              },
              {
                "name: "Song 2",
                "notes": "These are the song 2 notes."
            ],
            "name": "Christmas Show",
            songs: [
              {
                "name": "Song 2"
              },
              {
                "name": "Song name"
              },
              {
                "name: "Another Song"
              }
            ]
          }
        ]
      }
  */

class SetlistSong : public SerializableObject {
public:
  SetlistSong() {}
  virtual ~SetlistSong() {}

  const std::string& GetName() const { return name; }
  const std::string& GetNotes() const { return notes; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  std::string name;
  std::string notes;
};


typedef std::list<SetlistSong*> SetlistSongs;

class Setlist : public SerializableObject {
public:
  Setlist() {}
  virtual ~Setlist() {}

  const std::string& GetName() const { return name; }
  const SetlistSongs& GetSongs() const { return setlistSongs; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  std::string name;
  SetlistSongs setlistSongs;
};

typedef std::list<Setlist*> Sets;

class Setlists : public SerializableObject {
public:
  Setlists() {}
  virtual ~Setlists();

  const Sets& GetSets() const { return sets; }

  // Removes the specified setlist from our list so that we don't
  // free it when this object is destroyed. This is used when moving
  // from the setlist selection screen to the setlist screen, and allows
  // the setlist object to be passed from one screen to the other,
  // giving the new screen ownership (and responsibility for deallocation
  // of) the object.
  void EraseSetlist(Setlist *setlist) {
    sets.remove(setlist);
  }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  Sets sets;
};

} // namespace Serializable

#endif
