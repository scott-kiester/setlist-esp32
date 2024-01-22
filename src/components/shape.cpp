#include "tftmanager.hpp"
#include "components/shape.hpp"


///////////////////////////////////////////////////////////////////////////////
// Class Shape
///////////////////////////////////////////////////////////////////////////////
Shape::Shape(CanvasState& canvasState, ShapeType _shapeType, TSPoint _p1, TSPoint _p2):
  ComponentCanvas(canvasState),
  shapeType(_shapeType),
  p1(_p1),
  p2(_p2) {}


Shape::~Shape() {}


void Shape::Draw() {
  CanvasState canvasState;
  ourCanvasState.Apply();

  TFT_eSPI *tft = TftManager::GetTft();

  switch(shapeType) {
  case ShapeType::Line:
    tft->drawLine(p1.x, p1.y, p2.x, p2.y, ourCanvasState.fgColor);
    break;

  case ShapeType::Rect:
    tft->drawRect(p1.x, p1.y, p2.x, p2.y, ourCanvasState.fgColor);
    break;
  }
}


///////////////////////////////////////////////////////////////////////////////
// Class WideLine
///////////////////////////////////////////////////////////////////////////////
WideLine::WideLine(CanvasState& canvasState, TSPoint _p1, TSPoint _p2, float _width):
  ComponentCanvas(canvasState),
  p1(_p1),
  p2(_p2),
  width(_width) {}


WideLine::~WideLine() {}


void WideLine::Draw() {
  CanvasState canvasState;
  ourCanvasState.Apply();
  TftManager::GetTft()->drawWideLine(p1.x, p1.y, p2.x, p2.y, width, ourCanvasState.fgColor, ourCanvasState.bgColor);
}
