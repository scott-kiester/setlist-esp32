#include "components/button.hpp"
#include "log.hpp"
#include "tftmanager.hpp"

#define MIN_PRESS_TIME 150


Button::Button(CanvasState& canvasState, uint16_t _width, uint16_t _height, const char *_text):
  ComponentCanvas(canvasState),
  lastPressedTime(0),
  released(true),
  text(_text),
  width(_width),
  height(_height),
  outlineColor(canvasState.bgColor),
  fillColor(canvasState.bgColor) {
    DC_VAR_SET(initCalled, false);
  }


Button::~Button() {}


void Button::SetColors(uint16_t _outlineColor, uint16_t _fillColor) {
  outlineColor = _outlineColor;
  fillColor = _fillColor;
}


void Button::Init() {
  DC_VAR_SET(initCalled, true);

  CanvasState canvasState;
  ourCanvasState.Apply();

  logPrintf(LOG_COMP_BUTTON, LOG_SEV_VERBOSE, "initButtonUL: x: %d y: %d w: %d h: %d oc: %d fc: %d fgc: %d ts: %d\n",
    ourCanvasState.cursorX, ourCanvasState.cursorY, width, height, outlineColor, fillColor, ourCanvasState.fgColor,
    ourCanvasState.textSize);

  tftButton.initButtonUL(
    TftManager::GetTft(),
    ourCanvasState.cursorX,
    ourCanvasState.cursorY,
    width,
    height,
    outlineColor,
    fillColor,
    ourCanvasState.fgColor,
    const_cast<char*>(""),
    ourCanvasState.textSize);
}


void Button::Draw() {
  DC_ASSERT(initCalled != false);
  tftButton.drawButton(false, text);
}


void Button::Run(TSPoint *point) {
  if (point && !point->Equals(TSPoint(0, 0, 0)) && tftButton.contains(point->x, point->y)) {
    handlePress();
  } else {
    released = true;
  }
}


// Allow for sending touch events to the flashOnOff button manually, 
// only in the case that nothing else is triggered. That allows the
// user to toggle the flash by just touching the screen.
void Button::ManualState(bool pressed) {
  if (pressed) {
    handlePress();
  } else {
    released = true;
  }
}


void Button::handlePress() {
  // Don't count another press if the user hasn't lifted their finger.
  // Also don't allow rapid subsequent presses to avoid "phantom" 
  // button presses.
  if (released) {
    TIME_TYPE now = millis();
    if (lastPressedTime + MIN_PRESS_TIME < now) {
      lastPressedTime = now;
      released = false;
      OnPress();
    }
  }
}

