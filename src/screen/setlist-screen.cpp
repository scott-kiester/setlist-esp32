#include "audio.hpp"
#include "audio/player.hpp"
#include "components/button.hpp"
#include "Free_Fonts.h"
#include "log.hpp"
#include "screen/setlist-screen.hpp"
#include "tftmanager.hpp"


#define SONGS_FILE_PATH SDCARD_ROOT"/songs.json"
#define SONGS_MAX_SIZE (32 * 1024)

#define BOTTOM_ROW_NUM_ITEMS 4
#define BOTTOM_ROW_HEIGHT 50
#define BOTTOM_ROW_WIDTH TftManager::Width()
#define BOTTOM_ROW_ITEM_WIDTH (BOTTOM_ROW_WIDTH / BOTTOM_ROW_NUM_ITEMS)

#define RIGHT_COLUMN_WIDTH 75
#define RIGHT_COLUMN_NUM_ITEMS 4

#define RIGHT_COLUMN_ITEM_HEIGHT ((TftManager::Height() - BOTTOM_ROW_HEIGHT)  / RIGHT_COLUMN_NUM_ITEMS)

#define SWAP_GRID_BUTTON_HEIGHT RIGHT_COLUMN_ITEM_HEIGHT
#define VOL_UP_BUTTON_HEIGHT RIGHT_COLUMN_ITEM_HEIGHT
#define VOL_DOWN_BUTTON_HEIGHT RIGHT_COLUMN_ITEM_HEIGHT
#define METRONOME_BUTTON_HEIGHT RIGHT_COLUMN_ITEM_HEIGHT

#define SET_LIST_MAX_ROWS 11
#define SET_LIST_WIDTH (TftManager::Width() - RIGHT_COLUMN_WIDTH)
#define SET_LIST_HEIGHT (TftManager::Height() - BOTTOM_ROW_HEIGHT)
#define SET_LIST_MAX_NAME_LEN 40
#define SET_LIST_MAX_BPM_LEN 3

#define SONG_DETAIL_TITLE_HEIGHT 50

#define VOLUME_CHANGE 0.05

///////////////////////////////////////////////////////////////////////////////
// class SetlistSong
///////////////////////////////////////////////////////////////////////////////
SetlistSong::SetlistSong():
  song(NULL) {}


bool SetlistSong::SetSong(const Serializable::Song *_song, const Serializable::SetlistSong *_setlistSong) {
  try {
    song = _song;
    setlistSong = _setlistSong;
    bpm = std::to_string(_song->GetBPM());
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "SetlistSong::SetSong: song: %s, BPM: %s\n", song->GetName().c_str(), bpm.c_str());
    return true;
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Error storing setlist song data\n");
    return false;
  }
}


#define SWAP_GRID_BUTTON_TEXT_GRID "List"
#define SWAP_GRID_BUTTON_TEXT_SONG_DETAIL "Song"

///////////////////////////////////////////////////////////////////////////////
// class VolumeUpButton
///////////////////////////////////////////////////////////////////////////////
class SwapGridButton : public Button {
public:
  SwapGridButton(CanvasState& cs, uint16_t _width, uint16_t _height, SetlistScreen *_screen):
    Button(cs, _width, _height, SWAP_GRID_BUTTON_TEXT_SONG_DETAIL),
    screen(_screen) {}

  virtual ~SwapGridButton() {}

  virtual void OnPress() {
    if (strcmp(GetText(), SWAP_GRID_BUTTON_TEXT_GRID) == 0) {
      SetText(SWAP_GRID_BUTTON_TEXT_SONG_DETAIL);
    } else {
      SetText(SWAP_GRID_BUTTON_TEXT_GRID);
    }

    // This will call ManualDraw(), which is why we change the text first
    screen->SwapGridArea();
  }

private:
  SetlistScreen *screen;
};



///////////////////////////////////////////////////////////////////////////////
// class VolumeUpButton
///////////////////////////////////////////////////////////////////////////////
class VolumeUpButton : public Button {
public:
  VolumeUpButton(CanvasState& cs, uint16_t _width, uint16_t _height, TextBox *_volText):
    Button(cs, _width, _height, "Vol+"),
    volText(_volText) {}

  virtual ~VolumeUpButton() {}

  virtual void OnPress() {
    AudioLib::Player& player = AudioLib::Player::GetPlayer();
    uint16_t volume = static_cast<uint16_t>(player.SetVolume((player.GetVolume() + VOLUME_CHANGE)) * 100);
    volText->Update(std::string("Vol: ").append(std::to_string(volume)));
  }

private:
  TextBox *volText;
};



///////////////////////////////////////////////////////////////////////////////
// class VolumeDownButton
///////////////////////////////////////////////////////////////////////////////
class VolumeDownButton : public Button {
public:
  VolumeDownButton(CanvasState& cs, uint16_t _width, uint16_t _height, TextBox *_volText):
    Button(cs, _width, _height, "Vol-"),
    volText(_volText) {}

  virtual ~VolumeDownButton() {}

  virtual void OnPress() {
    AudioLib::Player& player = AudioLib::Player::GetPlayer();
    uint16_t volume = static_cast<uint16_t>(player.SetVolume((player.GetVolume() - VOLUME_CHANGE)) * 100);
    volText->Update(std::string("Vol: ").append(std::to_string(volume)));
  }

private:
  TextBox *volText;
};


///////////////////////////////////////////////////////////////////////////////
// class LightOnOffButton
///////////////////////////////////////////////////////////////////////////////
class LightOnOffButton : public Button {
public:
  LightOnOffButton(CanvasState& cs, uint16_t _width, uint16_t _height):
    Button(cs, _width, _height, "Light"),
    lightOn(true) {}

  virtual ~LightOnOffButton() {}

  virtual void OnPress() {
    lightOn = !lightOn;
    if (lightOn) {
      AudioComp::StartFlash();
    } else {
      AudioComp::StopFlash();
    }
  }

  virtual void Draw() {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "******** LightOnOffButton::Draw\n");
    Button::Draw();
  }

private:
  bool lightOn;
};



///////////////////////////////////////////////////////////////////////////////
// class ClickOnOffButton
///////////////////////////////////////////////////////////////////////////////
class ClickOnOffButton : public Button {
public:
  ClickOnOffButton(CanvasState& cs, uint16_t _width, uint16_t _height):
    Button(cs, _width, _height, "Click"),
    clickOn(true) {}

  virtual ~ClickOnOffButton() {}

  virtual void OnPress() {
    clickOn = !clickOn;
    if (clickOn) {
      AudioComp::RestartClick();
    } else {
      AudioComp::StopClick();
    }
  }

private:
  bool clickOn;
};



///////////////////////////////////////////////////////////////////////////////
// class TempoUpButton
///////////////////////////////////////////////////////////////////////////////
class TempoUpButton : public Button {
public:
  TempoUpButton(CanvasState& cs, uint16_t _width, uint16_t _height, SetlistScreen *_setlistScreen):
    Button(cs, _width, _height, "Tempo+"),
    setlistScreen(_setlistScreen) {}

  virtual ~TempoUpButton() {}

  virtual void OnPress() {
    setlistScreen->SetTempo(setlistScreen->GetTempo() + 1);
  }

private:
  SetlistScreen *setlistScreen;
};



///////////////////////////////////////////////////////////////////////////////
// class TempoDownButton
///////////////////////////////////////////////////////////////////////////////
class TempoDownButton : public Button {
public:
  TempoDownButton(CanvasState& cs, uint16_t _width, uint16_t _height, SetlistScreen *_setlistScreen):
    Button(cs, _width, _height, "Tempo-"),
    setlistScreen(_setlistScreen) {}

  virtual ~TempoDownButton() {}

  virtual void OnPress() {
    setlistScreen->SetTempo(setlistScreen->GetTempo() - 1);
  }

private:
  SetlistScreen *setlistScreen;
};




///////////////////////////////////////////////////////////////////////////////
// class SetlistScreen
///////////////////////////////////////////////////////////////////////////////
SetlistScreen::SetlistScreen():
  mainComp(NULL),
  setListBox(NULL),
  swapGridButton(NULL),
  songDetailPanel(NULL),
  songDetailTitle(NULL),
  songDetailNotes(NULL),
  nextPrevButtons(NULL),
  volTextbox(NULL),
  volUpButton(NULL),
  volDownButton(NULL),
  lightOnOffButton(NULL),
  clickOnOffButton(NULL),
  tempoUpButton(NULL),
  tempoDownButton(NULL),
  tempoTextBox(NULL),
  setlist(NULL),
  songStartIndex(0),
  nextPageIdx(-1),
  prevPageIdx(-1) {}


SetlistScreen::~SetlistScreen() {}


SetlistScreen* SetlistScreen::GetSetlistScreen() {
  static SetlistScreen setlistScreen;
  return &setlistScreen;
}


void SetlistScreen::Init() {
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Deserialize song list\n");
  if (!allSongs.DeserializeObject(SONGS_FILE_PATH, SONGS_MAX_SIZE)) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Failed to deserialize master song list\n");
    return;
  }

  if (!AudioComp::SetClickFile(SDCARD_ROOT"/metronome/click.wav")) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Unable to process click file\n");
  }
}


void SetlistScreen::Show() {
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "SetlistScreen::Show\n");

  try {
    // Set up a vector with all the songs for quick and easy access
    for (const Serializable::SetlistSong *serSetlistSong : setlist->GetSongs()) {
      const std::string& songName = serSetlistSong->GetName();
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Setlist song: %s\n", songName.c_str());

      bool found = false;
      for (const Serializable::Song* song : allSongs.GetSongs()) {
        if (songName.compare(song->GetName()) == 0) {
          SetlistSong *setlistSong = new SetlistSong();
          if (!setlistSong) {
            logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Unable to allocate SetlistSong for song: %s\n", songName.c_str());
            return;
          }

          setlistSong->SetSong(song, serSetlistSong);
          setlistSongs.push_back(setlistSong);

          found = true;
          break;
        }
      }

      if (!found) {
        logPrintf(LOG_COMP_SCREEN, LOG_SEV_WARN, "SetlistScreen: Song \"%s\" not found in master song list\n", serSetlistSong->GetName().c_str());
      }
    }
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "SetlistScreen: Out of memory building song list");
    return;
  }

  // The ComponentSelector will allow us to switch between the grid list and the detailed song view.
  mainComp = new ComponentSelector();
  if (!mainComp || !pushComponent(reinterpret_cast<Component**>(&mainComp))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to create and push resource for mainComp\n");
    return;
  }

  CanvasState cs(false);
  cs.cursorX = 0;
  cs.cursorY = 0;
  cs.fgColor = TFT_WHITE;
  cs.bgColor = TFT_BLACK;
  cs.freeFont = FF17;

  setListBox = new GridListBox(cs);
  if (!setListBox || !pushComponent(reinterpret_cast<Component**>(&setListBox))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to create and push resource for setListBox\n");
    return;
  }

  setListBox->Init(2, SET_LIST_MAX_ROWS);
  setListBox->SetWidthAndHeight(SET_LIST_WIDTH, SET_LIST_HEIGHT);
  setListBox->SetSelectedTextColor(TFT_BLACK, TFT_WHITE);

  if (!setListBox->AllocColumn(0, SET_LIST_MAX_NAME_LEN, 90)
    || !setListBox->AllocColumn(1, SET_LIST_MAX_BPM_LEN, 10)) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc columns for setListBox\n");
    return;
  }

  // Since we've called pushComponent on setListBox, it will be deallocated when this screen is hidden.
  mainComp->AddComponent(setListBox);
  mainComp->SetSelectedComponent(setListBox);

  // Add the song detail screen, to be switched out with the grid list box.
  songDetailPanel = new ComponentPanel(cs, SET_LIST_WIDTH, SET_LIST_HEIGHT);
  if (!songDetailPanel || !pushComponent(reinterpret_cast<Component**>(&songDetailPanel))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc columns for songDetailPanel\n");
    return;
  }

  songDetailPanel->Init();

  cs.freeFont = FF22;
  songDetailTitle = new TextBox(cs, SET_LIST_WIDTH, SONG_DETAIL_TITLE_HEIGHT);
  if (!songDetailTitle || !pushComponent(reinterpret_cast<Component**>(&songDetailTitle))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc columns for songDetailTitle\n");
    return;
  }

  songDetailTitle->Init();
  songDetailPanel->AddComponent(songDetailTitle);
  mainComp->AddComponent(songDetailPanel);

  cs.cursorY = SONG_DETAIL_TITLE_HEIGHT;
  cs.textWrap = true;
  cs.freeFont = FF17;
  songDetailNotes = new TextBox(cs, SET_LIST_WIDTH, SET_LIST_HEIGHT - SONG_DETAIL_TITLE_HEIGHT);
  if (!songDetailNotes || !pushComponent(reinterpret_cast<Component**>(&songDetailNotes))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc columns for songDetailNotes\n");
    return;
  }

  cs.textWrap = false;

  songDetailNotes->Init();
  songDetailPanel->AddComponent(songDetailNotes);

  nextPageIdx = 0;
  initSongGrid(nextPageIdx);

  nextPrevButtons = new NextPrevButtons();
  if (!nextPrevButtons || !pushComponent(reinterpret_cast<Component**>(&nextPrevButtons))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc nextPrevButtons\n");
    return;    
  }

  nextPrevButtons->Init(this, 0, RIGHT_COLUMN_WIDTH, 0, BOTTOM_ROW_HEIGHT);

  cs.cursorX = TftManager::Width() - RIGHT_COLUMN_WIDTH;
  cs.cursorY = 0;

  volTextbox = new TextBox(cs, RIGHT_COLUMN_WIDTH, RIGHT_COLUMN_ITEM_HEIGHT / 2);
  if (!volTextbox || !pushComponent(reinterpret_cast<Component**>(&volTextbox))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc volTextbox\n");
    return;    
  }

  volTextbox->Init();

  // Mulitply the volume by 100 so it's not a decimal. Also make sure we don't print a floating point number. 
  volTextbox->SetText(std::string("Vol: ").append(std::to_string(static_cast<uint16_t>(AudioLib::Player::GetPlayer().GetVolume() * 100)).c_str()));

  cs.cursorY += RIGHT_COLUMN_ITEM_HEIGHT / 2;
  tempoTextBox = new TextBox(cs, RIGHT_COLUMN_WIDTH, RIGHT_COLUMN_ITEM_HEIGHT / 2);
  if (!volTextbox || !pushComponent(reinterpret_cast<Component**>(&tempoTextBox))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc tempoTextBox\n");
    return;    
  }

  tempoTextBox->Init();
  //tempoTextBox->SetText(std::string("Tmpo: "));

  cs.cursorY += RIGHT_COLUMN_ITEM_HEIGHT / 2;
  swapGridButton = new SwapGridButton(cs, RIGHT_COLUMN_WIDTH, RIGHT_COLUMN_ITEM_HEIGHT, this);
  if (!swapGridButton || !pushComponent(reinterpret_cast<Component**>(&swapGridButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc columns for swapGridButton\n");
    return;
  }

  swapGridButton->SetColors(TFT_WHITE, TFT_BLUE);
  swapGridButton->Init();

  cs.cursorY += RIGHT_COLUMN_ITEM_HEIGHT;
  volUpButton = new VolumeUpButton(cs, RIGHT_COLUMN_WIDTH, RIGHT_COLUMN_ITEM_HEIGHT, volTextbox);
  if (!volUpButton || !pushComponent(reinterpret_cast<Component**>(&volUpButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc volUpButton\n");
    return;    
  }

  volUpButton->SetColors(TFT_WHITE, TFT_BLUE);
  volUpButton->Init();

  cs.cursorY += VOL_UP_BUTTON_HEIGHT;
  volDownButton = new VolumeDownButton(cs, RIGHT_COLUMN_WIDTH, RIGHT_COLUMN_ITEM_HEIGHT, volTextbox);
  if (!volDownButton || !pushComponent(reinterpret_cast<Component**>(&volDownButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc volDownButton\n");
    return;    
  }

  volDownButton->SetColors(TFT_WHITE, TFT_BLUE);
  volDownButton->Init();

  cs.cursorX = 0;
  cs.cursorY = TftManager::Height() - BOTTOM_ROW_HEIGHT;
  lightOnOffButton = new LightOnOffButton(cs, BOTTOM_ROW_ITEM_WIDTH, BOTTOM_ROW_HEIGHT);
  if (!lightOnOffButton || !pushComponent(reinterpret_cast<Component**>(&lightOnOffButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc lightOnOffButton\n");
    return;    
  }

  lightOnOffButton->SetColors(TFT_WHITE, TFT_BLUE);

  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "***** lightOnOffButton->Init()\n");
  lightOnOffButton->Init();


  cs.cursorX += BOTTOM_ROW_ITEM_WIDTH;
  clickOnOffButton = new ClickOnOffButton(cs, BOTTOM_ROW_ITEM_WIDTH, BOTTOM_ROW_HEIGHT);
  if (!clickOnOffButton || !pushComponent(reinterpret_cast<Component**>(&clickOnOffButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc clickOnOffButton\n");
    return;    
  }

  clickOnOffButton->SetColors(TFT_WHITE, TFT_BLUE);
  clickOnOffButton->Init();


  cs.cursorX += BOTTOM_ROW_ITEM_WIDTH;
  tempoDownButton = new TempoDownButton(cs, BOTTOM_ROW_ITEM_WIDTH, BOTTOM_ROW_HEIGHT, this);
  if (!tempoDownButton || !pushComponent(reinterpret_cast<Component**>(&tempoDownButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc tempoDownButton\n");
    return;    
  }

  tempoDownButton->SetColors(TFT_WHITE, TFT_BLUE);
  tempoDownButton->Init();


  cs.cursorX += BOTTOM_ROW_ITEM_WIDTH;
  tempoUpButton = new TempoUpButton(cs, BOTTOM_ROW_ITEM_WIDTH, BOTTOM_ROW_HEIGHT, this);
  if (!tempoUpButton || !pushComponent(reinterpret_cast<Component**>(&tempoUpButton))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc tempoUpButton\n");
    return;    
  }

  tempoUpButton->SetColors(TFT_WHITE, TFT_BLUE);
  tempoUpButton->Init();


  selectionChanged();
  Component::ManualDraw();
}


void SetlistScreen::Hide() {
  Screen::Hide();
  if (setlist) {
    delete setlist;
    setlist = NULL;
  }
}


void SetlistScreen::initSongGrid(uint16_t firstSongIdx) {
  uint8_t rowNum = 0;
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "firstsongIdx: %d, Songlist num listbox rows: %d, Num songs: %d\n", firstSongIdx, SET_LIST_MAX_ROWS, setlistSongs.size());

  setListBox->ClearValidRows();

  uint16_t songNum;
  uint8_t iterNum = 0;
  for (songNum = firstSongIdx; songNum < setlistSongs.size() && iterNum < SET_LIST_MAX_ROWS; songNum++, iterNum++) {
    const SetlistSong *song = setlistSongs[songNum];    
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Song: %s, BPM: %s\n", song->GetSong()->GetName().c_str(), song->GetBPM().c_str());

    if (setListBox->InsertRow() == 0xff) {
      // Not a fatal error, but it does mean we won't display all the bands.
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Row limit exceeded for setListBox\n");
      break;
    }
    
    setListBox->SetData(0, rowNum, song->GetSong()->GetName().c_str());
    setListBox->SetData(1, rowNum++, song->GetBPM().c_str());

    if (rowNum >= SET_LIST_MAX_ROWS) {
      break;
    }
  }

  if (firstSongIdx != 0) {
    // We're not on the first page, so there should be at least a full page of songs
    // behind it. Rewind to the start of the previous page.
    prevPageIdx = firstSongIdx - SET_LIST_MAX_ROWS;
    if (prevPageIdx < 0) {
      // If we get here, it's a bug
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_WARN, "BUG: Error computing previous page idx.\n");
      prevPageIdx = -1;
    }
  } else {
    prevPageIdx = -1;
  }

  if (songNum + 1 < setlistSongs.size()) {
    // There's a page after this one
    nextPageIdx = songNum + 1;
  } else {
    nextPageIdx = -1;
  }

  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "initSongGrid finished. nextPageIdx: %d, prevPageIdx: %d\n", nextPageIdx, prevPageIdx);

  songStartIndex = firstSongIdx;
}


bool SetlistScreen::NextPage() {
  bool pageExists = false;
  if (nextPageIdx != -1) {
    pageExists = true;
    initSongGrid(nextPageIdx);
    setListBox->SelectFirstItem();
    selectionChanged();
    Component::ManualDraw();
  }

  return pageExists;
}


bool SetlistScreen::PrevPage() {
  bool pageExists = false;
  if (prevPageIdx != -1) {
    pageExists = true;
    initSongGrid(prevPageIdx);
    setListBox->SelectLastItem();
    selectionChanged();
    Component::ManualDraw();
  }

  return pageExists;
}


void SetlistScreen::selectionChanged() {
  uint8_t selection = setListBox->GetSelectionIndex();
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "selectionChanged: selection: %d, songStartIndex: %d, setlistSongs.size(): %d\n", 
    selection, songStartIndex, setlistSongs.size());

  SetlistSong *selectedSong = setlistSongs[songStartIndex + selection];
  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "selectionChanged: name: %s, notes: %s\n", 
    selectedSong->GetSong()->GetName(), selectedSong->GetSetlistSong()->GetNotes());

  curTempo = selectedSong->GetSong()->GetBPM();
  AudioComp::StartClick(curTempo);
  //tempoTextBox->Update(std::string("Tempo: ").append(std::to_string(curTempo)));
  tempoTextBox->Update(std::to_string(curTempo));

  logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Setting songDetailTitle to %s\n", selectedSong->GetSong()->GetName().c_str());
  songDetailTitle->SetText(selectedSong->GetSong()->GetName());
  songDetailNotes->SetText(selectedSong->GetSetlistSong()->GetNotes());

  // There probably should be an "Update" method in Component for this, but for now I'm doing it the hard way.
  songDetailPanel->ClearComponent();
  songDetailPanel->Draw();
}


bool SetlistScreen::SelectNextItem() {
  bool ret = setListBox->SelectNextItem();
  if (ret) {
    selectionChanged();
  } else {
    // We're probably at the end
    ret = NextPage();
  }

  return ret;
}


bool SetlistScreen::SelectPrevItem() {
  bool ret = setListBox->SelectPrevItem();
  if (ret) {
    selectionChanged();
  } else {
    // We're probably at the beginning
    ret = PrevPage();
  }

  return ret;
}


bool SetlistScreen::SwapGridArea() {
  if (mainComp->GetSelectedComponent() == setListBox) {
    mainComp->SetSelectedComponent(songDetailPanel);
  } else {
    mainComp->SetSelectedComponent(setListBox);
  }

  Component::ManualDraw();
  return true;
}


void SetlistScreen::SetTempo(uint16_t newTempo) {
  if (newTempo > 300) {
    return;
  }

  curTempo = newTempo;
  AudioComp::StartClick(curTempo);
  //tempoTextBox->Update(std::string("Tempo: ").append(std::to_string(curTempo)));
  tempoTextBox->Update(std::to_string(curTempo));
}
