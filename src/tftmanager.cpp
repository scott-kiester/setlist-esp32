#include "Free_Fonts.h"

#include "components/canvasstate.hpp"
#include "log.hpp"
#include "storage/sdcard.hpp"
#include "tftmanager.hpp"

#define MINPRESSURE 350
#define PRESS_TIME 100

#define WIDTH 480
#define HEIGHT 320

#define CALIBRATION_FILE SDCARD_ROOT"/tft-calibration.bin"
#define CALIBRATION_DATA_LEN 5

SetlistTft* tft = NULL;


// len specifies size in bytes, not items
bool readCalibrationFile(uint16_t *calibData, uint8_t len) {
  bool success = false;

  logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "Opening calibration file for read\n");
  FILE *calibFile = fopen(CALIBRATION_FILE, "rb");
  if (calibFile) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "Reading from calibration file\n");
    size_t read = fread(calibData, len, 1, calibFile);
    if (read >= 1) {
      success = true;
    } else {
      logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "Unable to read expected length from TFT calibration file. Read: %d, expected: %d\n", read, len);  
    }

    fclose(calibFile);
  } else {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "Unable to open calibration file. This might be expected.\n");
  }

  return success;
}


bool writeCalibrationFile(uint16_t *calibData, uint8_t len) {
  bool success = false;

  logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "Opening calibration file for write\n");
  FILE *calibFile = fopen(CALIBRATION_FILE, "wb");
  if (calibFile) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "Writing %d bytes to calibration file\n", len);
    size_t written = fwrite(calibData, len, 1, calibFile);
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "Wrote %d bytes to calibration file\n", len);
    if (written >= 1) {
      success = true;
    } else {
      logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "TFT calibration file not written: %d\n", written);  
    }

    fclose(calibFile);
  } else {
    logLn(LOG_COMP_SDCARD, LOG_SEV_ERROR, "Unable to write TFT calibration file");
  }

  return success;
}


SetlistTft::SetlistTft() {
  setFreeFont(NULL); // Parent doesn't initialize this
}

SetlistTft::~SetlistTft() {}

const GFXfont* SetlistTft::getGfxFont() {
  return gfxFont;
}


void TftManager::Init() {
  tft = new SetlistTft();
  if (!tft) {
    logLn(LOG_COMP_SCREEN, LOG_SEV_ERROR, "FATAL: Unable to allocate TFT object");
    return;
  }

  logLn(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Initializing TFT");
  tft->init();
  tft->setRotation(3);
  tft->fillScreen(TFT_BLACK);

  logLn(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "TFT initialized");
}


void TftManager::Calibrate() {
  logLn(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Calibrating TFT");

  uint16_t calData[CALIBRATION_DATA_LEN];
  if (readCalibrationFile(calData, sizeof(calData))) {
    logLn(LOG_COMP_SDCARD, LOG_SEV_INFO, "Loaded calibration data from SD card");
    tft->setTouch(calData);
  } else {
    // Do calibration
    CanvasState canvasState;
    tft->setCursor(20, 0);
    tft->setTextFont(2);
    tft->setTextSize(1);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    tft->println("Touch corners as indicated");

    tft->setTextFont(1);
    tft->println();

    tft->calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    if (writeCalibrationFile(calData, sizeof(calData))) {
      logLn(LOG_COMP_SDCARD, LOG_SEV_INFO, "Calibration data saved");
    }

    canvasState.DumpCanvasState();
  }

  tft->fillScreen(TFT_BLACK);
  tft->setFreeFont(FF18);
  tft->setTextSize(1);
}


SetlistTft* TftManager::GetTft() {
  return tft;
}


uint16_t TftManager::Width() { 
  return WIDTH; 
}


uint16_t TftManager::Height() {
  return HEIGHT;
}


// Returns true if the screen was touched.
bool TftManager::GetTouchCoords(TSPoint *coords) {
  bool touched = false;
  uint16_t x = 0, y = 0;
  if (tft->getTouch(&x, &y, MINPRESSURE)) {
    touched = true;

    if (coords) {
      coords->x = x;
      coords->y = y;
    }

    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Touch detected: x: %d, y: %d\n", x, y);
  }

  return touched;
}
