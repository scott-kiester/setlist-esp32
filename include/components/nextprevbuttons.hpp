#ifndef __NEXT_PREV_BUTTONS_HPP___
#define __NEXT_PREV_BUTTONS_HPP___

#include "components/canvasstate.hpp"

// Generic Next and Previous buttons. Often they are invisible and trigger a scroll
// up/down action when the top or bottom of the screen is tapped.

class NextButton;
class PrevButton;

// Interface that must be implemented by classes using these buttons.
class InterfaceNextPrevButtonHost {
public:
  virtual bool SelectNextItem() = 0;
  virtual bool SelectPrevItem() = 0;
};


// Class that owns the buttons. An instance of this class should be a member of the screen
// using the buttons.
class NextPrevButtons : public Component {
public:
  NextPrevButtons();
  virtual ~NextPrevButtons();

  bool Init(
    InterfaceNextPrevButtonHost *parent, 
    uint16_t offsetLeft, 
    uint16_t offsetRight, 
    uint16_t offsetTop,
    uint16_t offsetBottom);

private:
  NextButton *nextButton;
  PrevButton *prevButton;
};


#endif
