#ifndef __SETLIST_SCREEN_HPP___
#define __SETLIST_SCREEN_HPP___

#include <vector>

#include "components/componentpanel.hpp"
#include "components/componentselector.hpp"
#include "components/gridlistbox.hpp"
#include "components/nextprevbuttons.hpp"
#include "components/textbox.hpp"
#include "screen/screen.hpp"
#include "serializable/setlists.hpp"
#include "serializable/songs.hpp"


class SwapGridButton;
class VolumeUpButton;
class VolumeDownButton;

class LightOnOffButton;
class ClickOnOffButton;

class TempoUpButton;
class TempoDownButton;

class SetlistSong {
public:
  SetlistSong();
  virtual ~SetlistSong() {}

  bool SetSong(const Serializable::Song *_song, const Serializable::SetlistSong *_setlistSong);
  const Serializable::Song* GetSong() const { return song; }
  const Serializable::SetlistSong* GetSetlistSong() const { return setlistSong; }
  const std::string& GetBPM() const { return bpm; }

private:
  const Serializable::Song *song;
  const Serializable::SetlistSong *setlistSong;
  std::string bpm;
};


typedef std::vector<SetlistSong*> SetlistSongs;

class SetlistScreen : public Screen, InterfaceNextPrevButtonHost {
public:
  SetlistScreen();
  virtual ~SetlistScreen();

  void SetSetlist(Serializable::Setlist *_setlist) { setlist = _setlist; }

  virtual void Init();
  virtual void Show();
  virtual void Hide();

  virtual bool SelectNextItem();
  virtual bool SelectPrevItem();

  bool NextPage();
  bool PrevPage();

  uint16_t GetTempo() { return curTempo; }
  void SetTempo(uint16_t newTempo);

  bool SwapGridArea();

  static SetlistScreen* GetSetlistScreen();

private:
  void initSongGrid(uint16_t firstSongIdx);
  void selectionChanged();

  ComponentSelector *mainComp;
  GridListBox *setListBox;
  SwapGridButton *swapGridButton;

  ComponentPanel *songDetailPanel;
  TextBox *songDetailTitle;
  TextBox *songDetailNotes;

  NextPrevButtons *nextPrevButtons;

  TextBox *volTextbox;
  VolumeUpButton *volUpButton;
  VolumeDownButton *volDownButton;

  LightOnOffButton *lightOnOffButton;
  ClickOnOffButton *clickOnOffButton;

  TempoUpButton *tempoUpButton;
  TempoDownButton *tempoDownButton;
  TextBox *tempoTextBox;

  Serializable::Setlist *setlist;
  Serializable::SongList allSongs;

  SetlistSongs setlistSongs;

  uint16_t songStartIndex;

  int16_t nextPageIdx;
  int16_t prevPageIdx;

  uint16_t curTempo;
};

#endif
