#include "components/canvasstate.hpp"
#include "log.hpp"
#include "components/textbox.hpp"
#include "tftmanager.hpp"


#define FILL_EXTRA_PIXELS 5


TextBox::TextBox(CanvasState& canvasState, uint16_t _height):
  ComponentCanvas(canvasState),
  reservedHeight(_height),
  fontHeight(0) {
  ourCanvasState.datum = TL_DATUM;
  ourCanvasState.cursorY += reservedHeight / 2; // Draw in the middle of the specified area
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
  text = _text;
}


void TextBox::Update(const char *newText) {
  text = newText;
  Draw();
}


void TextBox::Draw() {
  // Save the current state and apply ours.
  CanvasState canvasState;
  ourCanvasState.Apply();

  SetlistTft *tft = TftManager::GetTft();
  tft->fillRect(ourCanvasState.cursorX, ourCanvasState.cursorY, tft->textWidth(text) + tft->getTextPadding(), tft->fontHeight(), ourCanvasState.bgColor);
  tft->print(text);

  // Cache the font height for other components that will ask for it.
  fontHeight = tft->fontHeight();
}
