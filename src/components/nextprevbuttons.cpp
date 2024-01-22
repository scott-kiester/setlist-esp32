#include "components/button.hpp"
#include "components/nextprevbuttons.hpp"
#include "log.hpp"


///////////////////////////////////////////////////////////////////////////////
// NextButton
class NextButton: public Button {
public:
  NextButton(CanvasState& cs, uint16_t width, uint16_t height, InterfaceNextPrevButtonHost *_host):
    Button(cs, width, height, ""),
    host(_host) {}

  // Button will be invisible - will cover top half of screen.
  virtual void Draw() {}

  virtual void OnPress() {
    logPrintf(LOG_COMP_BUTTON, LOG_SEV_VERBOSE, "*** Next pressed!\n")
    host->SelectNextItem();
  }

private:
  InterfaceNextPrevButtonHost *host;
};


///////////////////////////////////////////////////////////////////////////////
// PrevButton
class PrevButton: public Button {
public:
  PrevButton(CanvasState& cs, uint16_t width, uint16_t height, InterfaceNextPrevButtonHost *_host):
    Button(cs, width, height, ""),
    host(_host) {}

  virtual void Draw() {}  

  virtual void OnPress() {
    logPrintf(LOG_COMP_BUTTON, LOG_SEV_VERBOSE, "*** Prev pressed!\n");
    host->SelectPrevItem();
  }

private:
  InterfaceNextPrevButtonHost *host;
};



///////////////////////////////////////////////////////////////////////////////
// NextPrevButtons
NextPrevButtons::NextPrevButtons():
  nextButton(NULL),
  prevButton(NULL) {}


NextPrevButtons::~NextPrevButtons() {
  if (nextButton) {
    delete nextButton;
  }

  if (prevButton) {
    delete prevButton;
  }
}


bool NextPrevButtons::Init(
  InterfaceNextPrevButtonHost *parent,
  uint16_t offsetLeft,
  uint16_t offsetRight,
  uint16_t offsetTop,
  uint16_t offsetBottom) {

  CanvasState cs(false);

  uint16_t width = (TftManager::Width() - (offsetLeft + offsetRight)) / 2;
  uint16_t height = (TftManager::Height() - (offsetTop + offsetBottom)) / 2;

  cs.cursorX = offsetLeft;
  cs.cursorY = offsetTop + height;
  nextButton = new NextButton(cs, width, height, parent);
  if (!nextButton) {
    logLn(LOG_COMP_BUTTON, LOG_SEV_ERROR, "NextPrevButtons: Unable to allocate next button");
    return false;
  }

  nextButton->Init();

  cs.cursorX = offsetLeft;
  cs.cursorY = offsetTop;
  prevButton = new PrevButton(cs, width, height, parent);
  if (!prevButton) {
    logLn(LOG_COMP_BUTTON, LOG_SEV_ERROR, "NextPrevButtons: Unable to allocate prev button");
    return false;
  }

  prevButton->Init();

  return true;
}