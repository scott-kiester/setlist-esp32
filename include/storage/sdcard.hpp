#ifndef __SDCARD_HPP___
#define __SDCARD_HPP___

#include <driver/sdmmc_types.h>
#include <FS.h>

#define SDCARD_ROOT "/sdcard"

class SdFs;

class SDCard {
private:
  SDCard(sdmmc_card_t *_card);

public:
  virtual ~SDCard();

  static void Init();
  static fs::FS* GetFS();

private:
  static bool init(sdmmc_card_t **card);

  sdmmc_card_t *card;
};

#endif
