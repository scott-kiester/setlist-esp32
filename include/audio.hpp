#ifndef __AUDIO_HPP___
#define __AUDIO_HPP___

#include <stdlib.h>

namespace AudioComp {
  void Init();

  bool PlayAudioFile(const char *fileName);

  bool SetClickFile(const char *fileName);
  bool StartClick(uint16_t bpm);
  bool StopClick();
}

#endif
