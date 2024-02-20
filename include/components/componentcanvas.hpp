#ifndef __COMPONENT_CANVAS_HPP___
#define __COMPONENT_CANVAS_HPP___

#include "components/canvasstate.hpp"
#include "components/component.hpp"


class ComponentCanvas : public Component {
public:
  ComponentCanvas(CanvasState& canvasState):
    ourCanvasState(canvasState) {}

  virtual ~ComponentCanvas() {}

  void ClearComponent(uint16_t width, uint16_t height);
  virtual void SetVisible(bool _visible) { ourCanvasState.visible = _visible; }

protected:
  CanvasState ourCanvasState;
};

#endif
