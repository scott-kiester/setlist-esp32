#include <list>

#include "components/component.hpp"
#include "log.hpp"
#include "screen/screen.hpp"
#include "tftmanager.hpp"


typedef std::list<Component*> ComponentList;
ComponentList components;

Component::Component() {
  components.push_back(this);
}


void Component::ClearComponentList() {
  components.clear();
}


void Component::MakeSelectable() {
  // Remove the component from the draw list, since the ComponentSelector will be
  // proxying the events.
  components.remove(this);
}


void Component::ManualDraw() {
  TftManager::GetTft()->fillScreen(TFT_BLACK);
  for (ComponentList::iterator it = components.begin(); it != components.end(); ++it) {
    Component *component = *it;
    component->Draw();
    logPrintf(LOG_COMP_COMPONENT, LOG_SEV_VERBOSE, "Draw component\n");
  }
}


void Component::Loop() {
  TSPoint pointTft(0, 0, 0);
  TftManager::GetTouchCoords(&pointTft);
  for (ComponentList::iterator it = components.begin(); it != components.end(); ++it) {
    Component *component = *it;
    component->Run(&pointTft);
  }
  
  // Now that we're done with the event loop, see if we need to change screens.
  ScreenManager::GetScreenManager()->ExecutePendingScreenChange();
}
