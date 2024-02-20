#ifndef __COMPONENT_PANEL_HPP___
#define __COMPONENT_PANEL_HPP___

#include <list>

#include "components/componentcanvas.hpp"


typedef std::list<Component*> PanelComponents;

// A container for multiple components, for use with ComponentSelector. This
// allows the song detail screen to contain mulitiple controls and be swapped
// out with the grid.
class ComponentPanel : public ComponentCanvas {
public:
  ComponentPanel(CanvasState& cs, uint16_t _width, uint16_t height);
  virtual ~ComponentPanel();

  virtual void Init();
  virtual void Draw();
  virtual void Run(TSPoint *point);

  void ClearComponent();

  // Caller's responsibility to ensure all components are drawn within the 
  // intended coords of the panel. There's nothing to stop components from
  // drawing wherever they please (and there's not likely to ever be), but 
  // things might look weird.
  bool AddComponent(Component *comp);

private:
  PanelComponents components;

  // TODO: These should be part of ComponentCanvas. Button has them also.
  uint16_t width;
  uint16_t height;
};

#endif
