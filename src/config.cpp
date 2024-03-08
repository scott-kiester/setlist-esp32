#include "config.hpp"
#include "log.hpp"

#define CONFIG_FILEPATH SDCARD_ROOT"/config.json"
#define CONFIG_FILE_MAX_SIZE 1024

std::shared_ptr<Config> config = NULL;
bool configFailed = false;


Config::~Config() {
  logPrintf(LOG_COMP_GENERAL, LOG_SEV_VERBOSE, "Config destructor called\n")
}


std::shared_ptr<const Config> Config::GetConfig() {
  if (configFailed) {
    // We failed to process it the first time. No point in trying again.
    return NULL;
  }

  if (!config) {
    config = GetConfigFrom(CONFIG_FILEPATH);
    if (!config) {
      configFailed = true;
    }
  }

  return config;
}


std::shared_ptr<Config> Config::GetConfigFrom(const std::string& filePath) {
  // Class to allow make_shared to be called without making the constructor public.
  // Thanks, Stackoverflow!
  class SharedPtrConstructor : public Config {};

  std::shared_ptr<Config> tmpConfig = std::make_shared<SharedPtrConstructor>();
  if (!tmpConfig) {
    logLn(LOG_COMP_GENERAL, LOG_SEV_ERROR, "Unable to allocate Config object");
    return NULL;
  }

  if (!tmpConfig->configData.DeserializeObject(filePath.c_str(), CONFIG_FILE_MAX_SIZE)) {
    logLn(LOG_COMP_GENERAL, LOG_SEV_ERROR, "Error reading config");
    return NULL;
  }

  logPrintf(LOG_COMP_GENERAL, LOG_SEV_INFO, "Config processed successfully\n");

  return tmpConfig;
}


void Config::UpdateConfig(const std::shared_ptr<Config> newConfig) {
  config = newConfig;
  configFailed = false;
}