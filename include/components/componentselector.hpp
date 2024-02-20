#ifndef __COMPONENT_SELECTOR_HPP___
#define __COMPONENT_SELECTOR_HPP___

#include <list>

#include "component.hpp"

typedef std::list<Component*> SelectorComponents;

// A class that allows easy switching between components that share the same
// display space. This is used to switch between the song grid and the song
// detail screen.
class ComponentSelector : public Component {
public:
  ComponentSelector();
  virtual ~ComponentSelector();

  virtual void Init();
  virtual void Draw();
  virtual void Redraw();

  virtual void Run(TSPoint *p);

  // Components can be added after initilization, but they must first be manually initialized
  bool AddComponent(Component *addThis);

  const Component* GetSelectedComponent() const { return selectedComponent; };
  const Component* SetSelectedComponent(Component *theComponent);

private:
  SelectorComponents selectorComponents;
  Component *selectedComponent;
};


#endif
