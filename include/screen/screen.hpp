#ifndef __SCREEN_HPP___
#define __SCREEN_HPP___

#include <list>

#include <components/component.hpp>

typedef std::list<Component**> ResourceList;

class ScreenManager;

class Screen {
public:
  Screen();
  virtual ~Screen();

  // One-time initialization. Allocate data necessary to maintain state.
  // Do not allocate display components here. Use Show() for that.
  virtual void Init() {}

  // Called to display the screen - either for the first time,
  // or to be shown again after being hidden.
  //
  // This is where display components should be allocated.
  virtual void Show() {}

  // Free up everything not necessary to maintain state, including display
  // components. This is done to preserve RAM, since we only have 8k
  // to work with.
  virtual void Hide() { freeComponents(); }

protected:
  bool pushComponent(Component **component);
  void freeComponents();

private:
  void doInitIfNeeded();

  ResourceList components;
  bool initNeeded;

friend class ScreenManager;
};


// Singleton class to handle screen changes
class ScreenManager {
public:
  ScreenManager();
  void ChangeScreen(Screen *newScreen);

  void ExecutePendingScreenChange();

  static ScreenManager* GetScreenManager();

private:
  Screen *curScreen;
  Screen *pendingScreen;
};

#endif
