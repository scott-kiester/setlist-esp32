#ifndef __SETLIST_SCREEN_HPP___
#define __SETLIST_SCREEN_HPP___

#include <vector>

#include "components/gridlistbox.hpp"
#include "components/nextprevbuttons.hpp"
#include "screen/screen.hpp"
#include "serializable/setlists.hpp"
#include "serializable/songs.hpp"


class SetlistSong {
public:
  SetlistSong();
  virtual ~SetlistSong() {}

  bool SetSong(const Serializable::Song *_song);
  const Serializable::Song* GetSong() const { return song; }
  const std::string& GetBPM() const { return bpm; }

private:
  const Serializable::Song *song;
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

  static SetlistScreen* GetSetlistScreen();

private:
  void selectionChanged();

  GridListBox *setListBox;
  NextPrevButtons *nextPrevButtons;

  Serializable::Setlist *setlist;
  Serializable::SongList allSongs;

  SetlistSongs setlistSongs;

  uint16_t songStartIndex;
};

#endif
