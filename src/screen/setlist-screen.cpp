#include "audio.hpp"
#include "components/button.hpp"
#include "Free_Fonts.h"
#include "log.hpp"
#include "screen/setlist-screen.hpp"
#include "tftmanager.hpp"


#define SONGS_FILE_PATH SDCARD_ROOT"/songs.json"
#define SONGS_MAX_SIZE (32 * 1024)

#define SET_LIST_MAX_ROWS 13
#define SET_LIST_WIDTH TftManager::Width()
#define SET_LIST_HEIGHT TftManager::Height()
#define SET_LIST_MAX_NAME_LEN 40
#define SET_LIST_MAX_BPM_LEN 3


///////////////////////////////////////////////////////////////////////////////
// class SetlistSong
///////////////////////////////////////////////////////////////////////////////
SetlistSong::SetlistSong():
  song(NULL) {
}


bool SetlistSong::SetSong(const Serializable::Song *_song) {
  try {
    song = _song;
    bpm = std::to_string(_song->GetBPM());
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "SetlistSong::SetSong: song: %s, BPM: %s\n", song->GetName().c_str(), bpm.c_str());
    return true;
  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Error storing setlist song data\n");
    return false;
  }
}



///////////////////////////////////////////////////////////////////////////////
// class SetlistScreen
///////////////////////////////////////////////////////////////////////////////
SetlistScreen::SetlistScreen():
  setListBox(NULL),
  nextPrevButtons(NULL),
  setlist(NULL),
  songStartIndex(0) {}


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
    for (const std::string &setlistSong : setlist->GetSongs()) {
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Setlist song: %s\n", setlistSong.c_str());

      bool found = false;
      for (const Serializable::Song* song : allSongs.GetSongs()) {
        if (setlistSong.compare(song->GetName()) == 0) {
          SetlistSong *setlistSong = new SetlistSong();
          if (!setlistSong) {
            logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Unable to allocate SetlistSong for song: %s\n", song->GetName().c_str());
            return;
          }

          setlistSong->SetSong(song);
          setlistSongs.push_back(setlistSong);

          found = true;
          break;
        }
      }

      if (!found) {
        logPrintf(LOG_COMP_SCREEN, LOG_SEV_WARN, "SetlistScreen: Song \"%s\" not found in master song list\n", setlistSong.c_str());
      }
    }
  } catch (std::bad_alloc&) {
    logLn(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "SetlistScreen: Out of memory building song list");
    return;
  }

  CanvasState cs(false);
  cs.cursorX = 0;
  cs.cursorY = 0;
  cs.fgColor = TFT_LIGHTGREY;
  cs.bgColor = TFT_BLACK;
  cs.freeFont = FF17;

  setListBox = new GridListBox(cs);
  if (!setListBox || !pushComponent(reinterpret_cast<Component**>(&setListBox))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to create and push resource for setListBox\n");
    return;
  }

  setListBox->Init(2, SET_LIST_MAX_ROWS);
  setListBox->SetWidthAndHeight(SET_LIST_WIDTH, SET_LIST_HEIGHT);
  setListBox->SetSelectedTextColor(TFT_WHITE, TFT_BLACK);

  if (!setListBox->AllocColumn(0, SET_LIST_MAX_NAME_LEN, 90)
    || !setListBox->AllocColumn(1, SET_LIST_MAX_BPM_LEN, 10)) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc columns for setListBox\n");
    return;
  }

  uint16_t rowNum = 0;
  for (const SetlistSong *song : setlistSongs) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_VERBOSE, "Song: %s, BPM: %s\n", song->GetSong()->GetName().c_str(), song->GetBPM().c_str());

    if (setListBox->InsertRow() == 0xff) {
      // Not a fatal error, but it does mean we won't display all the bands.
      logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "Row limit exceeded for setListBox\n");
      break;
    }
    
    setListBox->SetData(0, rowNum, song->GetSong()->GetName().c_str());
    setListBox->SetData(1, rowNum++, song->GetBPM().c_str());
  }

  selectionChanged();

  nextPrevButtons = new NextPrevButtons();
  if (!nextPrevButtons || !pushComponent(reinterpret_cast<Component**>(&nextPrevButtons))) {
    logPrintf(LOG_COMP_SCREEN, LOG_SEV_ERROR, "****** OUT OF MEMORY ******* Unable to alloc nextPrevButtons\n");
    return;    
  }

  nextPrevButtons->Init(this, 0, 0, 0, 0);

  Component::ManualDraw();
}


void SetlistScreen::Hide() {
  Screen::Hide();
  if (setlist) {
    delete setlist;
    setlist = NULL;
  }
}


void SetlistScreen::selectionChanged() {
  uint8_t selection = setListBox->GetSelectionIndex();
  SetlistSong *selectedSong = setlistSongs[songStartIndex + selection];
  AudioComp::StartClick(selectedSong->GetSong()->GetBPM());
}


bool SetlistScreen::SelectNextItem() {
  bool ret = setListBox->SelectNextItem();
  if (ret) {
    selectionChanged();
  }
  return ret;
}


bool SetlistScreen::SelectPrevItem() {
  bool ret = setListBox->SelectPrevItem();
  if (ret) {
    selectionChanged();
  }
  return ret;
}