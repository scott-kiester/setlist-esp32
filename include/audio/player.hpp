#ifndef __PLAYER_HPP___
#define __PLAYER_HPP___

#include <memory>

#include "audio/audiodata.hpp"


namespace AudioLib {

class Player {
private:
  Player();

public:
  virtual ~Player();

  // Start playing the file specified by playThis.
  bool Play(AudioDataInterface* _playThis);

  // Start over playing the current sample
  bool Replay();

  // Update the device with more data from playThis. Should be called 
  // frequently in a loop.
  bool WriteToDevice();

  static void Init(uint8_t bckPin, uint8_t wsPin, uint8_t dataOutPin);
  static Player& GetPlayer();

private:
  bool processSamples();

  AudioDataInterface *playThis;
  const AudioSamples *samples;

  std::unique_ptr<uint8_t[]> processedSamples;
  uint32_t processedSamplesLen;
  uint32_t processedSamplesTotalLen;

  bool playing;
  uint32_t samplesIdx;
  float volume;
};

}

#endif
