#include <Arduino.h>
#include <esp_chip_info.h>

#include "audio.hpp"
#include "components/component.hpp"
#include "log.hpp"
#include "screen/band-chooser-screen.hpp"
#include "screen/setlist-screen.hpp"
#include "storage/sdcard.hpp"
#include "tftmanager.hpp"


/* For TFT_eSPI, below should be the contents of user_setup.h. PlatforIO
   helpfully nukes it every time it does a clean build.

#define ILI9486_DRIVER

#define TFT_INVERSION_OFF

#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10 // Chip select control pin
#define TFT_DC    2 // Data Command control pin
#define TFT_RST   1 // Reset pin (could connect to RST pin)

#define TOUCH_CS  9 // Chip select pin (T_CS) of touch screen

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// this will save ~20kbytes of FLASH
#define SMOOTH_FONT

#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
*/


void printChipInfo() {
  esp_chip_info_t chipInfo;
  esp_chip_info(&chipInfo);

  switch (chipInfo.model) {
    case CHIP_ESP32:
      logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "Model: ESP32");
      break;
    case CHIP_ESP32S2:
      logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "Model: ESP32-S2");
      break;
    case CHIP_ESP32S3:
      logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "Model: ESP32-S3");
      break;
    case CHIP_ESP32C3:
      logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "Model: ESP32-C3");
      break;
    case CHIP_ESP32H2:
      logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "Model: ESP32-H2");
      break;
    default:
      logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "Model: unknown");
  }

  logPrintf(LOG_COMP_GENERAL, LOG_SEV_INFO, "Revision: %d.%d\n", chipInfo.full_revision & 0xff00, chipInfo.full_revision & 0xff);
  logPrintf(LOG_COMP_GENERAL, LOG_SEV_INFO, "Cores: %d\n", chipInfo.cores);


  logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "Features:");
  if (chipInfo.features & CHIP_FEATURE_EMB_FLASH) {
    logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "  Embedded Flash");
  }

  if (chipInfo.features & CHIP_FEATURE_WIFI_BGN) {
    logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "  WiFi(BGN)");
  }

  if (chipInfo.features & CHIP_FEATURE_BLE) {
    logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "  BLE");
  }

  if (chipInfo.features & CHIP_FEATURE_BT) {
    logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "  BT Classic");
  }

  if (chipInfo.features & CHIP_FEATURE_IEEE802154) {
    logLn(LOG_COMP_GENERAL, LOG_SEV_INFO, "  IEEE802.155.4");
  }
}


// optional
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}


void loop(void *) {
  logPrintf(LOG_COMP_GENERAL, LOG_SEV_VERBOSE, "Starting component message loop\n");
  while(true) {
    Component::Loop();
    yield();
  }
}


//void setup() {
extern "C" void app_main() {
  Serial.begin(115200);

  Serial.println(F("Set List (Bluetooth) starting..."));

  logFlags = LOG_COMP_ALL; //LOG_COMP_BLUETOOTH | LOG_COMP_SCREEN | LOG_COMP_GRID | LOG_COMP_SDCARD;
  logSeverity = LOG_SEV_VERBOSE;
  logInit();

  printChipInfo();

  // TftManager must be initialized before SD Card.
  TftManager::Init();
  SDCard::Init();
  AudioComp::Init();
  
  TftManager::Calibrate();

  //ScreenManager::GetScreenManager()->ChangeScreen(SetlistScreen::GetSetlistScreen());
  ScreenManager::GetScreenManager()->ChangeScreen(BandChooserScreen::GetBandChooserScreen());

  xTaskCreate(&loop, "Loopage", 1024 * 32, NULL, 5, NULL);
}
