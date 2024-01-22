#ifndef __SET_CHOOSER_SCREEN_HPP___
#define __SET_CHOOSER_SCREEN_HPP___

#include "components/gridlistbox.hpp"
#include "components/nextprevbuttons.hpp"
#include "components/textbox.hpp"
#include "screen/screen.hpp"
#include "serializable/setlists.hpp"

namespace SetChooserScreenLocal {
  class GoButton;
}

class SetChooserScreen : public Screen, public InterfaceNextPrevButtonHost {
public:
  SetChooserScreen();
  virtual ~SetChooserScreen() {}

  bool SetBandNameAndPath(const std::string& _bandName, const std::string& _path);

  virtual void Show();

  virtual bool SelectNextItem();
  virtual bool SelectPrevItem();

  // Retrieves the setlist currently selected in the list box. Also removes
  // it from setLists, making the caller responsible for deallocating
  // the memory.
  Serializable::Setlist* GetSelectedSetlist();

  static SetChooserScreen* GetSetChooserScreen();

private:
  std::string bandName;
  std::string path;
  std::string title;

  Serializable::Setlists setLists;

  SetChooserScreenLocal::GoButton *goButton;
  TextBox *titleBox;
  GridListBox *setListBox;
  NextPrevButtons *nextPrevButtons;
};

#endif
