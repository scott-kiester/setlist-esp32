#ifndef __WAV_HPP___
#define __WAV_HPP___

namespace AudioLib {

// https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

class WavHeader {
public:
  WavHeader();
  virtual ~WavHeader() {}

  bool ReadFromBuffer(const uint8_t *buf, uint32_t len);
  void Dump();

  uint32_t GetSize();

  char riff[4]; // Expected to be "RIFF"
  uint32_t size;
  char waveId[4]; // Expected to be "WAVE"

  // fmt chunk
  char fmtId[4];     // Expected to be "fmt "
  uint32_t fmtSize;  // Size of the rest the chunk (after this field). Expected to be 18.
  
  uint16_t fmtTag; // 1 for PCM
  uint16_t numChannels;
  uint32_t samplesPerSecond;
  uint32_t bytesPerSecond;
  uint16_t blockAlign;
  uint16_t bitsPerSample;

  // Not present with PCM
  //uint16_t extensionSize;
  //uint16_t validBitsPerSample;
  //uint32_t channelMask; // Speaker position mask according to the link above
};


class WavData {
public:
  WavData();
  virtual ~WavData() {}

  // To be called when first reading data from WAV file. This call
  // will validate the data header.
  bool Init(uint8_t *_buf, uint32_t _bufLen);

  // For subsequent calls on the same WAV file - for updating with
  // chunks of data which follow the data provided in Init().
  bool Update(uint8_t *_buf, uint32_t _bufLen);

  const uint8_t* GetData() { return buf; }
  uint32_t GetDataLen() { return bufLen; }

private:
  char id[4]; // Expected to be "data"
  

  uint8_t *buf; // Pointer to the actual data samples - caller owns this buffer
  uint32_t bufLen;
};


} // namespace AudioLib

#endif
