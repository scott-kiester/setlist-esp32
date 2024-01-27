#ifndef __TEXTBOX_HPP___
#define __TEXTBOX_HPP___

#include <string>

#include "components/canvasstate.hpp"
#include "components/componentcanvas.hpp"
#include "tftmanager.hpp"

class TextBox : public ComponentCanvas {
public:
  TextBox(CanvasState& canvasState, uint16_t _height);
  TextBox(CanvasState& canvasState, uint16_t _width, uint16_t _height);
  virtual ~TextBox();

  virtual void Draw();
  uint16_t GetFontHeight();

  // Sets the text and invokes Draw()
  void Update(const char *newText);
  void Update(const std::string& newText) { Update(newText.c_str()); }

  const char* GetText();
  void SetText(const char *_text);
  void SetText(const std::string& _text) { SetText(_text.c_str()); }

private:
  uint16_t reservedHeight;
  int16_t fontHeight;
  std::string text;

  uint16_t width;
  uint16_t height;
};

#endif
