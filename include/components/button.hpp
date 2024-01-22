#ifndef __BUTTON_H___
#define __BUTTON_H___

#include "debugchecks.hpp"

#include "components/componentcanvas.hpp"
#include "tftmanager.hpp"

class Button: public ComponentCanvas {
public:
  Button(CanvasState& canvasState, uint16_t _width, uint16_t _height, const char *_text);
  virtual ~Button();

  void SetColors(uint16_t _outlineColor, uint16_t _fillColor);

  virtual void Init();
  virtual void Draw();
  virtual void Run(TSPoint *point);

  // Override to get notifications
  virtual void OnPress() = 0;

  // Allow for sending touch events to the flashOnOff button manually, 
  // only in the case that nothing else is triggered. That allows the
  // user to toggle the flash by just touching the screen.
  void ManualState(bool pressed);

private:
  void handlePress();

  TFT_eSPI_Button tftButton;
  TIME_TYPE lastPressedTime;
  bool released;

  const char* text;
  uint16_t width;
  uint16_t height;
  uint16_t outlineColor;
  uint16_t fillColor;

  DC_VAR_DECLARE(bool, initCalled);
};

#endif