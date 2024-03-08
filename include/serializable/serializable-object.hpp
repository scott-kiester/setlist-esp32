#ifndef __SERIALIZABLE_OBJECT_HPP___
#define __SERIALIZABLE_OBJECT_HPP___

#include <vector>

#include <ArduinoJson.hpp>

#include "log.hpp"
#include "storage/sdcard.hpp"

namespace Serializable {


class SerializableObject {
public:
  SerializableObject();
  virtual ~SerializableObject();

  bool IsValid() { return valid; }

  bool DeserializeObject(const char *filePath, uint32_t maxDocSize);

  virtual bool DeserializeSelf(const ArduinoJson::JsonObject& obj) = 0;

private:
  ArduinoJson::DynamicJsonDocument* doc;
  bool valid;
};



}

#endif
