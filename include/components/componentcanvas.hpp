#ifndef __COMPONENT_CANVAS_HPP___
#define __COMPONENT_CANVAS_HPP___

#include "components/canvasstate.hpp"
#include "components/component.hpp"


class ComponentCanvas : public Component {
public:
  ComponentCanvas(CanvasState& canvasState):
    ourCanvasState(canvasState) {}

  virtual ~ComponentCanvas() {}

protected:
  CanvasState ourCanvasState;
};

#endif
