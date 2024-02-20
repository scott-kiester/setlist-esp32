#include "components/componentselector.hpp"
#include "debugchecks.hpp"
#include "log.hpp"

///////////////////////////////////////////////////////////////////////////////
// namespace ComponentSelector
///////////////////////////////////////////////////////////////////////////////
ComponentSelector::ComponentSelector():
  selectedComponent(NULL) {}


ComponentSelector::~ComponentSelector() {}


void ComponentSelector::Init() {
  for (Component *comp : selectorComponents) {
    comp->Init();
  }
}


void ComponentSelector::Draw() {
  if (selectedComponent) {
    selectedComponent->Draw();
  }
}


void ComponentSelector::Redraw() {
  if (selectedComponent) {
    selectedComponent->Redraw();
  }
}


void ComponentSelector::Run(TSPoint *p) {
  if (selectedComponent) {
    selectedComponent->Run(p);
  }
}


bool ComponentSelector::AddComponent(Component *addThis) {
  try {
    selectorComponents.push_back(addThis);
    addThis->MakeSelectable();

    // If this is the first component, select it
    if (selectorComponents.size() == 1) {
      selectedComponent = addThis;
    } else {
      addThis->SetVisible(false);
    }

    return true;
  } catch (std::bad_alloc&) {
    return false;
  }
}


const Component* ComponentSelector::SetSelectedComponent(Component *selectThis) {
#ifdef DEBUG_CHECKS_ENABLED
  if (selectThis) {
    bool found = false;
    for (Component* comp : selectorComponents) {
      if (comp == selectThis) {
        found = true;
      }
    }

    if (!found) {
      logPrintf(LOG_COMP_COMPONENT, LOG_SEV_WARN, "BUG: ComponentSelector::SetSelectedComponent called with a component that hasn't been added!\n");
    }
  }
#endif

  Component *prevSelected = selectedComponent;
  selectedComponent = selectThis;
  
  prevSelected->SetVisible(false);
  selectedComponent->SetVisible(true);
  return prevSelected;
}
