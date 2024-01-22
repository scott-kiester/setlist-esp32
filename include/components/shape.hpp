#ifndef __SHAPE_HPP___
#define __SHAPE_HPP___

#include "componentcanvas.hpp"
#include "tspoint.hpp"


enum ShapeType {
  Line,
  Rect
};


class Shape : public ComponentCanvas {
public:
  Shape(CanvasState& canvasState, ShapeType _shapeType, TSPoint _p1, TSPoint _p2);
  virtual ~Shape();

  virtual void Draw();

private:
  ShapeType shapeType;
  TSPoint p1;
  TSPoint p2;
};


class WideLine : public ComponentCanvas {
public:
  WideLine(CanvasState& canvasState, TSPoint _p1, TSPoint _p2, float _width);
  virtual ~WideLine();

  virtual void Draw();

private:
  TSPoint p1;
  TSPoint p2;
  float width;
};

#endif
