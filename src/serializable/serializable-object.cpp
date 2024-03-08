#include <stdio.h>
#include <memory>
#include <fstream>

#include "serializable/serializable-object.hpp"
#include "tools/memorytools.hpp"


namespace Serializable {

SerializableObject::SerializableObject():
  doc(NULL),
  valid(false) {}


SerializableObject::~SerializableObject() {
  if (doc) {
    delete doc;
  }
}


bool SerializableObject::DeserializeObject(const char *filePath, uint32_t maxDocSize) {
  ArduinoJson::DynamicJsonDocument *doc = NULL;
  try {
    struct stat fileStat;
    if (stat(filePath, &fileStat) == -1) {
      logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_WARN, "Unable to stat file %s. Error: %d. Serialization failed.\n", filePath, errno);
      return false;
    }

    if (fileStat.st_size > maxDocSize) {
      logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, 
        "File is larger than max size allowed. Serialization failed.\n\tActual Size: %d, Max Size: %d\n", 
        fileStat.st_size, maxDocSize);
        return false;
    }

    std::ifstream docFile;
    docFile.open(filePath, std::ios::in);

    doc = new ArduinoJson::DynamicJsonDocument(maxDocSize);
    if (!doc) {
      logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Unble to allocate DynamicJsonDocument\n");
      return false;
    }

    ArduinoJson::DeserializationError error = ArduinoJson::deserializeJson(*doc, docFile);
    if (error != ArduinoJson::DeserializationError::Ok) {
      if (error == ArduinoJson::DeserializationError::NoMemory) {
        logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Got NoMemory error from ArduinoJson::deserializeJson. Dumping heap info:\n");
        Tools::PrintAllHeapFreeSizes(LOG_COMP_SERIALIZE, LOG_SEV_WARN);
      }

      logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_INFO, "Error deserializing JSON doc: %s. File: %s\n", error.c_str(), filePath);
      return false;
    }

    if (!DeserializeSelf(doc->as<ArduinoJson::JsonObject>())) {
        logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_INFO, "Error deserializing items in JSON doc: %s. File: %s\n", error.c_str(), filePath);
        return false;
    }

    valid = true;
  } catch (const std::exception& exc) {
    logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "\nCaught exception: %s\n\n", exc.what());
    valid = false;
  }

  if (doc) {
    delete doc;
  }

  return valid;
}

} // namespace Serializable
