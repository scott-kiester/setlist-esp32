#ifndef __CONFIG_HPP___
#define __CONFIG_HPP___

#include "serializable/band-data.hpp"
#include "serializable/config-data.hpp"


class Config {
public:
  Config() {}

public:
  virtual ~Config() {}

  const Serializable::ConfigData* GetConfigData() const { return &configData; }

  static const Config* GetConfig();

private:
  Serializable::ConfigData configData;
};

#endif
