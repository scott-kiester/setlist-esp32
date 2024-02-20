#ifndef __COMPONENT_H____
#define __COMPONENT_H____

#include "tspoint.hpp"

#define TIME_TYPE unsigned long

///////////////////////////////////////////////////////////////////////////////
// Component
class Component {
public:
  Component();
  virtual ~Component() {}

  virtual void Init() {}
  virtual void Run(TSPoint *p) {}
  virtual void Draw() {}

  virtual void SetVisible(bool _visible) {}

  // Same as Draw(), but only for a single component. The component may
  // need to clear its background first.
  virtual void Redraw() {}

  // To be called by ComponentSelector when it takes over management of a component.
  void MakeSelectable();

  static void ManualDraw();
  static void Loop();

  // Empties the event loop list. Should only be used when
  // moving to a different screen.
  static void ClearComponentList();
};

#endif
