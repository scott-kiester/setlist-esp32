#include "config.hpp"
#include "log.hpp"

#define CONFIG_FILEPATH SDCARD_ROOT"/config.json"
#define CONFIG_FILE_MAX_SIZE 1024

const Config* Config::GetConfig() {
  static bool configFailed = false;
  if (configFailed) {
    // We failed to process it the first time. No point in trying again.
    return NULL;
  }

  static Config *config = NULL;
  if (!config) {
    config = new Config();
    if (!config) {
      logLn(LOG_COMP_GENERAL, LOG_SEV_ERROR, "Unable to allocate Config object");
      return NULL;
    }

    if (!config->configData.DeserializeObject(CONFIG_FILEPATH, CONFIG_FILE_MAX_SIZE)) {
      logLn(LOG_COMP_GENERAL, LOG_SEV_ERROR, "Error reading config");
      delete config;
      config = NULL;
      configFailed = true;
      return NULL;
    }
  }

  return config;
}