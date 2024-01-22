#ifndef __MEMWAV_HPP___
#define __MEMWAV_HPP___

#include <memory>

#include "audio/audiodata.hpp"
#include "audio/wav.hpp"


namespace AudioLib {

// This class keeps a small .wav file in memory for quick
// repeated playback. It's used for the click sound.
class MemWav : public AudioDataInterface {
public:
  MemWav();
  virtual ~MemWav();

  bool InitFromFile(const char *fileName);
  bool Valid() { return valid; }

  // AudioDataInterface methods
  virtual bool HasMoreData() { return false; }
  virtual void Restart() {} // Nothing to do, since we always return the entire set of samples
  virtual uint32_t GetSampleRate();
  virtual uint16_t GetBitsPerSample();
  virtual const AudioSamples* GetSamples();

private:
  bool readFromFile(const char *fileName);

  std::unique_ptr<uint8_t[]> wavBuf;
  uint32_t wavBufLen;

  WavHeader wavHeader;
  WavData wavData;

  bool valid;

  AudioSamples samples;
};


} // namespace AudioLib

#endif