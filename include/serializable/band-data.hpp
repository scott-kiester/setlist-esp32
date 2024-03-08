#ifndef __BAND_DATA_HPP___
#define __BAND_DATA_HPP___

#include <list>
#include <string>

#include <ArduinoJson.hpp>

#include "serializable/serializable-object.hpp"


#define BAND_DATA_FILE_MAXSIZE (1024 * 2)

namespace Serializable {

/*
  JSON format:
  {
    bands: [
      {
        name: "Route 89",
        directory: "/route-89"
      },
      {
        name: "Twisted Hitch",
        directory: "/twisted-hitch"
      }
    ]
  }
*/

class BandData : public SerializableObject {
public:
  BandData() {}
  virtual ~BandData() {}

  const std::string& GetName() const { return name; }
  const std::string& GetPath() const { return path; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  std::string name;
  std::string path;
};


typedef std::list<BandData*> Bands;

class BandList : public SerializableObject {
public:
  BandList() {}
  virtual ~BandList();

  const Bands& GetBands() const { return bands; }

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj);

private:
  Bands bands;
};


}

#endif
