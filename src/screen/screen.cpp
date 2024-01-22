#include "components/component.hpp"
#include "log.hpp"
#include "screen/screen.hpp"
#include "tftmanager.hpp"



///////////////////////////////////////////////////////////////////////////////
// Screen
///////////////////////////////////////////////////////////////////////////////
Screen::Screen():
  initNeeded(true) {}


Screen::~Screen() {
  freeComponents();
}


void Screen::freeComponents() {
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Screen::freeComponents\n");
  for (Component **component : components) {
    if (*component) {
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Screen: Freeing component\n");
      delete (*component);
      (*component) = NULL; // This allows for automatically "NULLing out" the class member pointer
    }
  }

  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Screen: Done freeing components\n");
}


bool Screen::pushComponent(Component **component) {
  bool success = false;

  try {
    components.push_back(component);
    success = true;
  } catch (std::bad_alloc& ba) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to push screen resource\n");
  }

  return success;
}


void Screen::doInitIfNeeded() {
  if (initNeeded) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Doing one-time screen init\n");
    Init();
    initNeeded = false;
  } else {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Screen is already initialized\n");
  }
}



///////////////////////////////////////////////////////////////////////////////
// ScreenManager
///////////////////////////////////////////////////////////////////////////////
ScreenManager screenManager;

ScreenManager::ScreenManager():
  curScreen(NULL),
  pendingScreen(NULL) {}


ScreenManager* ScreenManager::GetScreenManager() {
  return &screenManager;
}


// We can't change screens during the event loop, or we risk
// freeing components that are still pending run in the loop.
// Queue the change to happen after the loop instead. 
void ScreenManager::ChangeScreen(Screen *newScreen) {
  pendingScreen = newScreen;
}


void ScreenManager::ExecutePendingScreenChange() {
  if (!pendingScreen) {
    return;
  }

  TftManager::GetTft()->fillScreen(TFT_BLACK);

  Component::ClearComponentList();

  if (curScreen) {
    curScreen->Hide();
  }

  curScreen = pendingScreen;
  pendingScreen = NULL;
  
  curScreen->doInitIfNeeded();
  curScreen->Show();
}
