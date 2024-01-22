#include "components/button.hpp"
#include "Free_Fonts.h"
#include "log.hpp"
#include "screen/band-chooser-screen.hpp"
#include "screen/set-chooser-screen.hpp"
#include "screen/screen.hpp"
#include "screen/setlist-screen.hpp"
#include "storage/sdcard.hpp"


#define SETLISTS_JSON_FILENAME "/sets.json"
#define SETLIST_MAX_DOC_SIZE (1024 * 16)


#define GO_BUTTON_HEIGHT (TftManager::Height())
#define GO_BUTTON_WIDTH 50
#define GO_BUTTON_X (TftManager::Width() - GO_BUTTON_WIDTH)
#define GO_BUTTON_Y 0

#define TITLE_BOX_HEIGHT (TftManager::Height() / 13)
#define TITLE_BOX_WIDTH (TftManager::Width() - BO_BUTTON_WIDTH)

#define SET_LIST_INDENT 15
#define SET_LIST_WIDTH (TftManager::Width() - (GO_BUTTON_WIDTH + SET_LIST_INDENT))
#define SET_LIST_HEIGHT (TftManager::Height() - TITLE_BOX_HEIGHT)
#define SET_LIST_MAX_ROWS 12
#define SET_LIST_MAX_NAME_LEN 25

#define BACK_BUTTON_HEIGHT 50
#define BACK_BUTTON_WIDTH 100

// Putting locally-defined classes in their own namespace avoids linker confusion
// when dealing with multiple classes of the same name.
namespace SetChooserScreenLocal {


///////////////////////////////////////////////////////////////////////////////
// SetChooserScreenLocal::GoButton
///////////////////////////////////////////////////////////////////////////////
class GoButton : public Button {
public:
  GoButton(CanvasState& canvasState, SetChooserScreen& _setChooserScreen):
    Button(canvasState, GO_BUTTON_WIDTH, GO_BUTTON_HEIGHT, "GO"),
    setChooserScreen(_setChooserScreen) {}

  virtual void OnPress() {
    Serializable::Setlist *setList = setChooserScreen.GetSelectedSetlist();
    logPrintf(LOG_COMP_BUTTON, LOG_SEV_VERBOSE, "*** GO pressed! (SetChooserScreen)\n");

    SetlistScreen *setlistScreen = SetlistScreen::GetSetlistScreen();
    setlistScreen->SetSetlist(setList);
    ScreenManager::GetScreenManager()->ChangeScreen(setlistScreen);
  }

private:
  SetChooserScreen& setChooserScreen;
};
}


///////////////////////////////////////////////////////////////////////////////
// class SetChooserScreen
///////////////////////////////////////////////////////////////////////////////

SetChooserScreen::SetChooserScreen():
  goButton(NULL),
  titleBox(NULL),
  setListBox(NULL),
  nextPrevButtons(NULL) {}


SetChooserScreen* SetChooserScreen::GetSetChooserScreen() {
  static SetChooserScreen setChooserScreen;
  return &setChooserScreen;
}


bool SetChooserScreen::SetBandNameAndPath(const std::string& _bandName, const std::string& _path) {
  try {
    bandName = _bandName;
    path = _path;
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "SetChooserScreen: Unable to allocate space for band name and path\n");
    return false;
  }

  return true;
}


void SetChooserScreen::Show() {
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "SetChooserScreen::Show\n");

  CanvasState cs(false);
  cs.cursorX = GO_BUTTON_X;
  cs.cursorY = GO_BUTTON_Y;
  cs.fgColor = TFT_LIGHTGREY;
  cs.bgColor = TFT_NAVY;
  cs.freeFont = FF17;

  std::string jsonFile;
  try {
    jsonFile = path;
    jsonFile.append(SETLISTS_JSON_FILENAME);
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SCREEN, LOG_SEV_ERROR, "SetChooserScreen: Out of memory building JSON file path");
    return;
  }

  if (!setLists.DeserializeObject(jsonFile.c_str(), SETLIST_MAX_DOC_SIZE)) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Unable to deserialize setlists JSON file: %s\n", jsonFile.c_str());
    return;
  }

  const Serializable::Sets& sets = setLists.GetSets();
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "SetChooserScreen: Num sets: %d\n", sets.size());

  if (sets.size() < 1) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "SetChooserScreen: Band %s does not have any sets\n", bandName.c_str());
    TftManager::GetTft()->printf("Band %s does not have any sets\n", bandName.c_str());
    class BackButton : public Button {
      public:
        BackButton(CanvasState& cs):
          Button(cs, BACK_BUTTON_HEIGHT, BACK_BUTTON_WIDTH, "Go Back") {}
        
        virtual void OnPress() {
          ScreenManager::GetScreenManager()->ChangeScreen(BandChooserScreen::GetBandChooserScreen());
        }
    };

    BackButton *backButton = new BackButton(cs);
    if (backButton) {
      Component *comp = backButton;
      pushComponent(&comp); // Parent will free when screen is hidden
    }

    return;
  } else {
    // If there's only one set, then select it and go

  }

  goButton = new SetChooserScreenLocal::GoButton(cs, *this);
  if (!goButton || !pushComponent(reinterpret_cast<Component**>(&goButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to create and push resource for go button\n");
    return;
  }

  goButton->Init();

  cs.cursorX = 0;
  cs.cursorY = 0;
  cs.fgColor = TFT_WHITE;
  cs.bgColor = TFT_BLACK;
  titleBox = new TextBox(cs, TITLE_BOX_HEIGHT);
  if (!titleBox || !pushComponent(reinterpret_cast<Component**>(&titleBox))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to create and push title box for setListBox\n");
    return;
  }

  try {
    title = "Sets for ";
    title.append(bandName);
    titleBox->Update(title.c_str());
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to allocate title string for setListBox\n");
    return;
  }

  cs.cursorX = SET_LIST_INDENT;
  cs.cursorY = TITLE_BOX_HEIGHT;
  cs.fgColor = TFT_LIGHTGREY;
  cs.bgColor = TFT_BLACK;
  setListBox = new GridListBox(cs);
  if (!setListBox || !pushComponent(reinterpret_cast<Component**>(&setListBox))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to create and push resource for setListBox\n");
    return;
  }

  setListBox->Init(1, SET_LIST_MAX_ROWS);
  setListBox->SetWidthAndHeight(SET_LIST_WIDTH, SET_LIST_HEIGHT);
  setListBox->SetSelectedTextColor(TFT_WHITE, TFT_BLACK);

  if (!setListBox->AllocColumn(0, SET_LIST_MAX_NAME_LEN, 100)) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc column bandListBox\n");
    return;
  }

  uint16_t rowNum = 0;
  for (const Serializable::Setlist *set : sets) {
    const char* setName = set->GetName().c_str();
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Set: %s\n", setName);

    if (setListBox->InsertRow() == 0xff) {
      // Not a fatal error, but it does mean we won't display all the sets.
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Row limit exceeded for setListBox\n");
      break;
    }
    
    setListBox->SetData(0, rowNum++, setName);
  }

  nextPrevButtons = new NextPrevButtons();
  if (!nextPrevButtons || !pushComponent(reinterpret_cast<Component**>(&nextPrevButtons))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc nextPrevButtons\n");
    return;    
  }

  nextPrevButtons->Init(this, 0, GO_BUTTON_WIDTH, 0, 0);

  Component::ManualDraw();
}


bool SetChooserScreen::SelectNextItem() {
  return setListBox->SelectNextItem();
}


bool SetChooserScreen::SelectPrevItem() {
  return setListBox->SelectPrevItem();
}


Serializable::Setlist* SetChooserScreen::GetSelectedSetlist() {
  Serializable::Setlist *theSetlist = NULL;
  const char *nameStr = setListBox->GetSelectedCellData(0);

  // This is inefficient, but the data set should always be small.
  for (Serializable::Setlist* sl : setLists.GetSets()) {
    if (sl->GetName().compare(nameStr) == 0) {
      theSetlist = sl;
      setLists.EraseSetlist(theSetlist);
      break;
    }
  }

  if (!theSetlist) {
    // If we get here, then something is seriously wrong
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "SetChooserScreen: Selected setlist not found! Setlist: %s\n", nameStr);
  }

  return theSetlist;
}