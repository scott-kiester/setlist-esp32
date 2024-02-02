#ifndef __AUDIO_HPP___
#define __AUDIO_HPP___

#include <stdlib.h>

namespace AudioComp {
  void Init();

  bool PlayAudioFile(const char *fileName);

  bool SetClickFile(const char *fileName);
  bool StartClick(uint16_t bpm);
  bool RestartClick(uint32_t startTime = 0);

  bool StopClick();
  bool StartFlash();
  bool StopFlash();
}

#endif
