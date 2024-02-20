#include "components/canvasstate.hpp"
#include "log.hpp"
#include "components/textbox.hpp"
#include "tftmanager.hpp"


#define FILL_EXTRA_PIXELS 5


TextBox::TextBox(CanvasState& canvasState, uint16_t _height):
  ComponentCanvas(canvasState),
  reservedHeight(_height),
  fontHeight(0),
  width(0),
  height(0) {
    ourCanvasState.datum = TL_DATUM;
}

TextBox::TextBox(CanvasState& canvasState, uint16_t _width, uint16_t _height):
  ComponentCanvas(canvasState),
  reservedHeight(_height),
  fontHeight(0),
  width(_width),
  height(_height) {
    ourCanvasState.datum = TL_DATUM;
}


TextBox::~TextBox() {}


uint16_t TextBox::GetFontHeight() {
  if (fontHeight != 0) {
    return fontHeight;
  }
  
  // We don't have it cached. Compute it.
  // Just perform a draw to get it.
  Draw();
  return fontHeight;  
}


void TextBox::SetText(const char *_text) {
  try {
    text = _text;
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "bad_alloc exception setting button text to %s\n", _text);
  }
}


void TextBox::Update(const char *newText) {
  try {
    text = newText;
    Draw();
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "bad_alloc exception setting button text to %s\n", newText);
  }
}


void TextBox::Draw() {
  // Save the current state and apply ours.
  CanvasState canvasState;
  ourCanvasState.Apply();

  SetlistTft *tft = TftManager::GetTft();

  uint16_t fillX, fillY, fillWidth, fillHeight;

  if (width == 0 && height == 0) {
    fillX = ourCanvasState.cursorX;
    fillWidth = tft->textWidth(text.c_str()) + tft->getTextPadding();
    fillHeight = tft->fontHeight();

    fillY = ourCanvasState.cursorY;
    if (fillY >= tft->fontHeight()) {
      fillY -= tft->fontHeight();
    } else {
      // Bug
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_WARN, "Clearing textbox: cursorY (%d) is less than font height (%d)\n", fillY, tft->fontHeight());
      fillY = 0;
    }
  } else {
    fillX = ourCanvasState.cursorX;
    fillY = ourCanvasState.cursorY - reservedHeight / 2;
    fillWidth = width;
    fillHeight = height;
  }

  tft->fillRect(fillX, fillY, fillWidth, fillHeight, ourCanvasState.bgColor);

  if (ourCanvasState.textWrap) {
    // Text wraps inside the viewport
    // (I could probably take better advatage of the viewport feature. I didn't notice it until now.)
    tft->setViewport(ourCanvasState.cursorX, ourCanvasState.cursorY, width, height);

    // Begin writing in the middle (vertical) of the first line.
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Textbox: text wraps. cursorY: %d, tft->fontHeight() / 2: %d\n", 
      ourCanvasState.cursorY, tft->fontHeight() / 2);
    tft->setCursor(0, tft->fontHeight() / 2);
  } else {
    // Begin writing in the center, rather than at the top
    tft->setCursor(ourCanvasState.cursorX, ourCanvasState.cursorY + reservedHeight / 2);
  }

  tft->print(text.c_str());
  tft->resetViewport();

  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Textbox filled background rect with color: %d, X: %d, Y: %d, Width: %d, Height: %d\n",
    ourCanvasState.bgColor, fillX, fillY, fillWidth, fillHeight);

  // Cache the font height for other components that will ask for it.
  fontHeight = tft->fontHeight();
}
