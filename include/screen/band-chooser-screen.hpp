#ifndef __BAND_CHOOSER_SCREEN_HPP___
#define __BAND_CHOOSER_SCREEN_HPP___

#include "components/gridlistbox.hpp"
#include "components/nextprevbuttons.hpp"
#include "screen/screen.hpp"
#include "serializable/band-data.hpp"

namespace BandChooserScreenLocal {
  class GoButton;
}

class BandChooserScreen : public Screen, public InterfaceNextPrevButtonHost {
public:
  BandChooserScreen();
  virtual ~BandChooserScreen();

  virtual void Init();
  virtual void Show();

  // Returns true if a new item was selected - see GridListBox
  bool SelectNextItem();
  bool SelectPrevItem();

  bool GetSelectedBandNameAndPath(std::string& bandName, std::string& bandPath);

  static BandChooserScreen* GetBandChooserScreen();

private:
  BandChooserScreenLocal::GoButton *goButton;
  NextPrevButtons *nextPrevButtons;
  GridListBox *bandListBox;

  Serializable::BandList bandList;
};

#endif