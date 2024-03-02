#include <components/button.hpp>
#include "Free_Fonts.h"
#include "log.hpp"
#include "screen/band-chooser-screen.hpp"
#include "screen/set-chooser-screen.hpp"
#include "network/wifi.hpp"

#define BAND_DATA_FILEPATH SDCARD_ROOT"/band-data.json"
#define BAND_DATA_FILE_MAXSIZE 1024

#define GO_BUTTON_HEIGHT (TftManager::Height())
#define GO_BUTTON_WIDTH 50
#define GO_BUTTON_X (TftManager::Width() - GO_BUTTON_WIDTH)
#define GO_BUTTON_Y 0

#define BAND_LIST_WIDTH (TftManager::Width() - GO_BUTTON_WIDTH)
#define BAND_LIST_HEIGHT (TftManager::Height())
#define BAND_LIST_MAX_ROWS 13
#define BAND_LIST_MAX_NAME_LEN 25


// Putting locally-defined classes in their own namespace avoids linker confusion
// when dealing with multiple classes of the same name.
namespace BandChooserScreenLocal {


///////////////////////////////////////////////////////////////////////////////
// BandChooserScreenLocal::GoButton
///////////////////////////////////////////////////////////////////////////////
class GoButton : public Button {
public:
  GoButton(CanvasState& canvasState, BandChooserScreen& _bandChooserScreen):
    Button(canvasState, GO_BUTTON_WIDTH, GO_BUTTON_HEIGHT, "GO"),
    bandChooserScreen(_bandChooserScreen) {}

  virtual void OnPress() {
    logPrintf(LOG_COMP_BUTTON, LOG_SEV_VERBOSE, "*** GO pressed! (BandChooserScreen)\n")

    std::string bandName;
    std::string path;
    if (bandChooserScreen.GetSelectedBandNameAndPath(bandName, path)) {
      SetChooserScreen *setChooserScreen = SetChooserScreen::GetSetChooserScreen();
      setChooserScreen->SetBandNameAndPath(bandName, path);

      logPrintf(LOG_COMP_BUTTON, LOG_SEV_VERBOSE, "Switching to SetChooserScreen: Band: %s, path: %s\n", bandName.c_str(), path.c_str());
      ScreenManager::GetScreenManager()->ChangeScreen(setChooserScreen);
    } else {
      logPrintf(LOG_COMP_BUTTON, LOG_SEV_ERROR, "Unable to find selected band in list. This should never happen.\n");
    }
  }

private:
  BandChooserScreen& bandChooserScreen;
};
}


///////////////////////////////////////////////////////////////////////////////
// BandChooserScreen
///////////////////////////////////////////////////////////////////////////////

BandChooserScreen::BandChooserScreen():
  goButton(NULL),
  nextPrevButtons(NULL),
  bandListBox(NULL) {}


BandChooserScreen::~BandChooserScreen() {}


BandChooserScreen* BandChooserScreen::GetBandChooserScreen() {
  static BandChooserScreen bandChooserScreen;
  return &bandChooserScreen;
}



void BandChooserScreen::Init() {
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Deserialize band list\n");
  bandList.DeserializeObject(BAND_DATA_FILEPATH, BAND_DATA_FILE_MAXSIZE);
  if (!bandList.IsValid()) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_WARN, "Unable to retrieve band list from file\n");
  }
}


void BandChooserScreen::Show() {
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "BandChooserScreen::Show\n");

  CanvasState cs(false);
  cs.cursorX = GO_BUTTON_X;
  cs.cursorY = GO_BUTTON_Y;
  cs.fgColor = TFT_LIGHTGREY;
  cs.bgColor = TFT_NAVY;
  cs.freeFont = FF17;
  goButton = new BandChooserScreenLocal::GoButton(cs, *this);
  if (!goButton || !pushComponent(reinterpret_cast<Component**>(&goButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to create and push resource for go button\n");
    return;
  }

  goButton->Init();

  cs.cursorX = 0;
  cs.cursorY = 0;
  cs.fgColor = TFT_LIGHTGREY;
  cs.bgColor = TFT_BLACK;
  bandListBox = new GridListBox(cs);
  if (!bandListBox || !pushComponent(reinterpret_cast<Component**>(&bandListBox))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to create and push resource for bandListBox\n");
    return;
  }

  bandListBox->Init(1, BAND_LIST_MAX_ROWS);
  bandListBox->SetWidthAndHeight(BAND_LIST_WIDTH, BAND_LIST_HEIGHT);
  bandListBox->SetSelectedTextColor(TFT_WHITE, TFT_BLACK);

  if (!bandListBox->AllocColumn(0, BAND_LIST_MAX_NAME_LEN, 100)) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc column bandListBox\n");
    return;
  }

  const Serializable::Bands& bands = bandList.GetBands();
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "BandChooserScreen: Num bands: %d\n", bands.size());

  uint16_t rowNum = 0;
  for (Serializable::Bands::const_iterator it = bands.begin(); it != bands.end(); ++it) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Band: %s\n", (*it)->GetName().c_str());

    if (bandListBox->InsertRow() == 0xff) {
      // Not a fatal error, but it does mean we won't display all the bands.
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Row limit exceeded for bandListBox\n");
      break;
    }
    
    bandListBox->SetData(0, rowNum++, (*it)->GetName().c_str());
  }

  nextPrevButtons = new NextPrevButtons();
  if (!nextPrevButtons || !pushComponent(reinterpret_cast<Component**>(&nextPrevButtons))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc nextPrevButtons\n");
    return;    
  }

  nextPrevButtons->Init(this, 0, GO_BUTTON_WIDTH, 0, 0);

  Component::ManualDraw();
}


bool BandChooserScreen::SelectNextItem() {
  return bandListBox->SelectNextItem();
}


bool BandChooserScreen::SelectPrevItem() {
  return bandListBox->SelectPrevItem();
}


bool BandChooserScreen::GetSelectedBandNameAndPath(std::string& bandName, std::string& path) {
  bool found = false;
  const char *nameStr = bandListBox->GetSelectedCellData(0);

  try {
    bandName = nameStr;

    // This is inefficient, but the data set should always be small.
    for (Serializable::BandData* bd : bandList.GetBands()) {
      if (bd->GetName().compare(bandName) == 0) {
        path = bd->GetPath();
        found = true;
        break;
      }
    }

    if (!found) {
      // If we get here, then something is seriously wrong
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "BandChooseScreen: Selected band not found! Band: %s\n", nameStr);
    }
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "bad_alloc copying band and path name: %s\n", nameStr);
  }

  return found;
}