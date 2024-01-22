#include <stdio.h>
#include <memory>

#include "serializable/serializable-object.hpp"

namespace Serializable {

SerializableObject::SerializableObject():
  doc(NULL),
  valid(false) {}


SerializableObject::~SerializableObject() {
  if (doc) {
    delete doc;
  }
}


bool SerializableObject::deserializeFromBuffer(std::vector<char>& rawData, FILE *docFile, const char* filePath, size_t fileSize) {
  if (fread(rawData.data(), fileSize, 1, docFile) < 1) {
    logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Error reading string to serialize JSON. File: %s\n", filePath);
    return false;
  }

  rawData[fileSize] = '\0';
  logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_VERBOSE, "Read string from file: %s\n", rawData.data());

  std::unique_ptr<ArduinoJson::DynamicJsonDocument> doc(new ArduinoJson::DynamicJsonDocument(rawData.capacity()));
  if (!doc) {
    logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Unble to allocate DynamicJsonDocument\n");
    return false;
  }

  ArduinoJson::DeserializationError error = ArduinoJson::deserializeJson(*doc, rawData.data());
  if (error != ArduinoJson::DeserializationError::Ok) {
    logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_INFO, "Error deserializing JSON doc: %s\n", filePath);
    return false;
  }

  if (!DeserializeSelf(doc->as<ArduinoJson::JsonObject>())) {
      logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_INFO, "Error deserializing items in JSON doc: %s\n", filePath);
      return false;
  }

  return true;
}


bool SerializableObject::DeserializeObject(const char *filePath, uint32_t maxDocSize) {
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

  std::vector<char> jsonStr;
  try {
    jsonStr.reserve(maxDocSize + 1);
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "std::bad_alloc exception resizing string for JSON doc. Size: %d\n", maxDocSize);
    return false;
  }

  ArduinoJson::DynamicJsonDocument *doc = new ArduinoJson::DynamicJsonDocument(maxDocSize);
  if (!doc) {
    logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_ERROR, "Unble to allocate DynamicJsonDocument\n");
    return false;
  }

  FILE *docFile = fopen(filePath, "r");
  if (docFile) {
    bool result = deserializeFromBuffer(jsonStr, docFile, filePath, fileStat.st_size);
    fclose(docFile);

    if (!result) {
      return false;
    }

  } else {
    logPrintf(LOG_COMP_SERIALIZE, LOG_SEV_INFO, "Unable to open json file: %s. This might be expected.\n", filePath);
    return false;
  }

  valid = true;
  return valid;
}

} // namespace Serializable
