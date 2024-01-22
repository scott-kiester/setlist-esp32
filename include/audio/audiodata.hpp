#ifndef __AUDIO_DATA_HPP___
#define __AUDIO_DATA_HPP___

namespace AudioLib {

class AudioSamples {
public:
  AudioSamples(): samples(NULL), len(0) {}
  virtual ~AudioSamples() {}

  const uint8_t *samples;
  uint32_t len;
};


// An interface to be used by the Player class for data retrival when
// playing. Initial implementation will be .wav, but an MP3 implementation
// is expected later.
class AudioDataInterface {
public:
  virtual bool HasMoreData() = 0;
  virtual void Restart() = 0;

  virtual uint32_t GetSampleRate() = 0;
  virtual uint16_t GetBitsPerSample() = 0;
  virtual const AudioSamples* GetSamples() = 0;
};

} // namespace AudioLib

#endif
