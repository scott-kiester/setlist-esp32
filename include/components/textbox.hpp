#ifndef __TEXTBOX_HPP___
#define __TEXTBOX_HPP___

#include "components/canvasstate.hpp"
#include "components/componentcanvas.hpp"
#include "tftmanager.hpp"

class TextBox : public ComponentCanvas {
public:
  TextBox(CanvasState& canvasState, uint16_t _height);
  virtual ~TextBox();

  virtual void Draw();

/*
  void SetCoords(uint16_t _x, uint16_t _y);
  void SetTextSize(uint8_t _textSize);
  void SetTextColor(uint16_t _fgColor, uint16_t _bgColor);

  void SetFont(const GFXfont *_font);
  */

  uint16_t GetFontHeight();

  // Sets the text and invokes Draw()
  void Update(const char *newText);

  const char* GetText();
  void SetText(const char *_text);

private:
  uint16_t reservedHeight;
  int16_t fontHeight;
  const char *text;
};

#endif
