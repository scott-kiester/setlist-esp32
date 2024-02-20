#include "components/componentpanel.hpp"
#include "log.hpp"

ComponentPanel::ComponentPanel(CanvasState& cs, uint16_t _width, uint16_t _height):
  ComponentCanvas(cs),
  width(_width),
  height(_height) {}


ComponentPanel::~ComponentPanel() {}


void ComponentPanel::Init() {
  for (Component *comp : components) {
    comp->Init();
  }
}


void ComponentPanel::Draw() {
  if (!ourCanvasState.visible) {
    return;
  }

  CanvasState origCanvasState;
  ourCanvasState.Apply();

  for (Component *comp : components) {
    logLn(LOG_COMP_COMPONENT, LOG_SEV_VERBOSE, "CompenentPanel::Draw() calling Draw() for component");
    comp->Draw();
  }
}


void ComponentPanel::Run(TSPoint *point) {
  for (Component *comp : components) {
    comp->Run(point);
  }
}


void ComponentPanel::ClearComponent() {
  ComponentCanvas::ClearComponent(width, height);
}


bool ComponentPanel::AddComponent(Component *comp) {
  // Should probably do some sanity checks here to ensure the component is within the
  // coords of the panel. For now that's up to the caller.
  //
  // I think I'm going to have to change some interfaces to make that happen. Argh.

  try {
    components.push_back(comp);
    comp->MakeSelectable();
    return true;
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_COMPONENT, LOG_SEV_ERROR, "Out of memory error in ComponentPanel::AddComponent");
    return false;
  }
}