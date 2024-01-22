#include "components/canvasstate.hpp"
#include "log.hpp"

CanvasState::CanvasState(bool _applyOnDestruction):
  applyOnDestruction(_applyOnDestruction) {

  SetlistTft *tft = TftManager::GetTft();

  cursorX = tft->getCursorX();
  cursorY = tft->getCursorY();

  textSize = tft->textsize;
  textPadding = tft->getTextPadding();
  fgColor = tft->textcolor;
  bgColor = tft->textbgcolor;
  datum = tft->getTextDatum();

  freeFont = tft->getGfxFont();
  textFont = tft->textfont;
}


CanvasState::CanvasState(CanvasState& canvasState) {
  applyOnDestruction = canvasState.applyOnDestruction;
  cursorX = canvasState.cursorX;
  cursorY = canvasState.cursorY;
  textSize = canvasState.textSize;
  textPadding = canvasState.textPadding;
  fgColor = canvasState.fgColor;
  bgColor = canvasState.bgColor;
  datum = canvasState.datum;
  freeFont = canvasState.freeFont;
  textFont = canvasState.textFont;
}


CanvasState::~CanvasState() {
  if (applyOnDestruction) {
    Apply();
  }
}


void CanvasState::Apply() {
  SetlistTft *tft = TftManager::GetTft();

  tft->setCursor(cursorX, cursorY);

  tft->setTextSize(textSize);
  tft->setTextPadding(textPadding);
  tft->setTextColor(fgColor, bgColor);
  tft->setTextDatum(datum);

  tft->setTextFont(textFont);

  if (freeFont) {
    tft->setFreeFont(freeFont);
  }
}


void CanvasState::DumpCanvasState() {
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_INFO, "cursorX: %d, cursorY: %d\n", cursorX, cursorY);
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_INFO, "testSize: %d\n", textSize);
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_INFO, "fgColor: %u, bgColor: %u\n", fgColor, bgColor);
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_INFO, "datum: %d\n", datum);
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_INFO, "freeFont addr: %u\n", freeFont);
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_INFO, "textFont: %d\n", textFont);
}
