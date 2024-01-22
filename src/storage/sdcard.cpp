#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>
#include <esp_vfs_fat.h>
#include <SPI.h>

#include <dirent.h>

#include "log.hpp"
#include "storage/sdcard.hpp"
#include "storage/sdcard-fs.hpp"

// ESP32-S3-DevKitC-1
/*
#define SD_CS   8
#define SD_MOSI 11 
#define SD_MISO 13
#define SD_SCK  12
*/

#define SD_CS   8
#define SD_MOSI 15
#define SD_MISO 16
#define SD_SCK  17


SDCard *sdCard = NULL;

void listDir(const char * dirName){
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "Listing directory: %s\n", dirName);

  DIR *dp = opendir(dirName);
  if (!dp) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "Error opening directory %s\n", dirName);
    return;
  }

  struct dirent *ep = NULL;
  do {
    ep = readdir(dp);
    if (ep) {
      logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "\t%s\n", ep->d_name);
    }
  } while(ep);

  closedir(dp);
}


SDCard::SDCard(sdmmc_card_t *_card):
  card(_card) {}


SDCard::~SDCard() {
  esp_vfs_fat_sdcard_unmount(SDCARD_ROOT, card);
}


bool SDCard::init(sdmmc_card_t **card) {
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();

  // The TFT/LCD is connected to SPI2. No point in sharing that bus since we have
  // another one that's free.
  host.slot = SPI3_HOST;
  host.max_freq_khz = SDMMC_FREQ_DEFAULT; 
  //host.max_freq_khz = 5000; 

  spi_bus_config_t busConfig = {
    .mosi_io_num = SD_MOSI,
    .miso_io_num = SD_MISO,
    .sclk_io_num = SD_SCK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .data4_io_num = -1,
    .data5_io_num = -1,
    .data6_io_num = -1,
    .data7_io_num = -1,
    .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    .flags = 0,
    .intr_flags = 0,
  };

  esp_err_t ret = spi_bus_initialize(static_cast<spi_host_device_t>(host.slot), &busConfig, SDSPI_DEFAULT_DMA);
  if (ret != ESP_OK) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "Error initializing SPI bus: %d\n", ret);
    return false;
  }

  sdspi_device_config_t deviceConfig = SDSPI_DEVICE_CONFIG_DEFAULT();
  deviceConfig.gpio_cs = static_cast<gpio_num_t>(SD_CS);
  deviceConfig.host_id = static_cast<spi_host_device_t>(host.slot);

  esp_vfs_fat_sdmmc_mount_config_t mountConfig = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024,
  };

  bool success = false;

  ret = esp_vfs_fat_sdspi_mount(SDCARD_ROOT, &host, &deviceConfig, &mountConfig, card);
  if (ret == ESP_OK) {
    success = true;
  } else {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "Failed to mount SD card. Error: %d\n", ret);
  }

  if (!success) {
    spi_bus_free(static_cast<spi_host_device_t>(host.slot));
  }

  return success;
}


void SDCard::Init() {
  sdmmc_card_t *card = NULL;
  bool success = false;
  for (int i = 0; !success && i < 10; i++) {
    success = init(&card);
    if (!success) {
      delay(i * 1000);
    }
  }

  if (!success) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "Giving up mounting SD card\n")
    return;
  }

  listDir(SDCARD_ROOT);

  sdCard = new SDCard(card);
  if (!sdCard) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "Unable to allocate SdCard object\n");
    return;
  }

  logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "SD card initialized\n");
}


fs::FS* SDCard::GetFS() {
  static fs::FS theFs(std::make_shared<SdCardFs>());
  return &theFs;
}