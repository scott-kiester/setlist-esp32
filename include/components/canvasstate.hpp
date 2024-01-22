#ifndef __CANVAS_STATE_HPP___
#define __CANVAS_STATE_HPP___

#include "tftmanager.hpp"

// Save the state of the TFT canvas during construction and
// restore it on destruction
class CanvasState {
public:
  // @param _applyOnDestruction: Applies the contained settings when this
  // object is destroyed. Useful for restoring canvas state on destruction.
  CanvasState(bool _applyOnDestruction = true);
  CanvasState(CanvasState& canvasState); // Copy constructor

  virtual ~CanvasState();

  // Set the canvas to the state stored in this object
  void Apply();

  // Write the state to the serial port
  void DumpCanvasState();

  bool applyOnDestruction;

  uint16_t cursorX;
  uint16_t cursorY;

  uint8_t textSize;
  uint16_t textPadding;
  uint32_t fgColor;
  uint32_t bgColor;
  uint8_t datum;

  const GFXfont *freeFont;
  uint8_t textFont;
};

#endif
