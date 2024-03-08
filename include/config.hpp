#ifndef __CONFIG_HPP___
#define __CONFIG_HPP___

#include <memory>

#include "serializable/band-data.hpp"
#include "serializable/config-data.hpp"


class Config {
private:
  Config() {}

public:
  virtual ~Config();

  const Serializable::ConfigData* GetConfigData() const { return &configData; }

  static std::shared_ptr<const Config> GetConfig();

  // Construct a standalone config from the specified path. This allows new configs
  // to be sanity-checked before they're used.
  static std::shared_ptr<Config> GetConfigFrom(const std::string& filePath);

  // The config object needs to be reference-counted. This call allows the 
  // main config to be changed to the config specified, which was created with 
  // GetConfigFrom().
  //
  // This call will be used when downloading new configs. It allows the new
  // config to be sanity-checked before switching to it. 
  static void UpdateConfig(std::shared_ptr<Config> newConfig);

private:
  Serializable::ConfigData configData;
};

#endif
