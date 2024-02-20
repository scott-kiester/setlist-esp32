#include "components/componentcanvas.hpp"
#include "tftmanager.hpp"

void ComponentCanvas::ClearComponent(uint16_t width, uint16_t height) {
  if (ourCanvasState.visible) {
    TftManager::GetTft()->fillRect(ourCanvasState.cursorX, ourCanvasState.cursorY, width, height, ourCanvasState.bgColor);
  }
}