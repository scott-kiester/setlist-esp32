#include <stdlib.h>

#include "audio/memwav.hpp"
#include "log.hpp"


namespace AudioLib {

#define WAV_HEADER_SIZE 36
#define WAV_FMT_SIZE 16

#define WAV_FMT_TAG_PCM 1



///////////////////////////////////////////////////////////////////////////////
// class WavHeader
///////////////////////////////////////////////////////////////////////////////
WavHeader::WavHeader() {
  memset(riff, 0, WAV_HEADER_SIZE);
}


void WavHeader::Dump() {
  // A buffer for logging 4-character strings
  char scratchBuf[5];
  scratchBuf[4] = '\0'; // Ensure it's NULL-terminated

  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "Dumping .wav header:\n");

  memcpy(scratchBuf, riff, 4);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tChunk ID: %s\n", scratchBuf);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tSize: %d\n", size);

  memcpy(scratchBuf, waveId, 4);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tWave ID: %s\n", scratchBuf);

  memcpy(scratchBuf, fmtId, 4);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tFmt ID: %s\n", scratchBuf);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tFmt Size: %d\n", fmtSize);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tFmt Tag: %d\n", fmtTag);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tNum Channels: %d\n", numChannels);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tSamples Per Second: %d\n", samplesPerSecond);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tBytes Per Second: %d\n", bytesPerSecond);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tBlock Align: %d\n", blockAlign);
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tBits Per Sample: %d\n", bitsPerSample);

  // Not present with PCM
  //logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tExtension Size: %d\n", extensionSize);
  //logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tValid Bits Per Sample: %d\n", validBitsPerSample);
  //logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "\tChannel Mask: %d\n", channelMask);
}


bool WavHeader::ReadFromBuffer(const uint8_t *buf, uint32_t len) {
  // Read the header (first 44 bytes) of the buffer and do some basic validation
  // to ensure we're dealing with the expected PCM data.
  if (len < WAV_HEADER_SIZE) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Expected a buffer of at least %d length when reading .wav file. Got only %d.\n", WAV_HEADER_SIZE, len);
    return false;    
  }

  // Copy and validate the data
  bool valid = true;

  // A buffer for logging 4-character strings
  char scratchBuf[5];
  scratchBuf[4] = '\0'; // Ensure it's NULL-terminated

  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "Validating WAV header...\n");

  memcpy(riff, buf, WAV_HEADER_SIZE);
  if (memcmp(riff, "RIFF", 4) != 0) {
    valid = false;
    memcpy(scratchBuf, riff, 4);
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Initial chunk ID invalid. Expected %s, got %s\n", "RIFF", scratchBuf);
  }

  if (size > len) {
    valid = false;
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Initial chunk size exceeds buffer size. Size: %d, buffer size: %d\n", size, len);
  }

  if (memcmp(waveId, "WAVE", 4) != 0) {
    valid = false;
    memcpy(scratchBuf, waveId, 4);
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Wave ID invalid. Expected %s, got %s\n", "WAVE", scratchBuf);
  }

  if (memcmp(fmtId, "fmt ", 4) != 0) {
    valid = false;
    memcpy(scratchBuf, fmtId, 4);
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Format ID invalid. Expected %s, got %s\n", "fmt ", scratchBuf);
  }

  if (fmtSize != WAV_FMT_SIZE) {
    valid = false;
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Format size invalid. Size: %d, expected: %d\n", fmtSize, WAV_FMT_SIZE);
  }

  if (fmtTag != WAV_FMT_TAG_PCM) {
    valid = false;
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Format tag invalid. (Not PCM encoding?) Got: %d, expected: %d\n", fmtTag, WAV_FMT_TAG_PCM);
  }

  if (numChannels != 2) {
    valid = false;
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Num channels invalid. Got: %d, expected: %d\n", numChannels, 2);
  }


  // TODO: Properly sanity check the remaining fields?

  logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "Done validating WAV header. Result is %s\n", valid ? "VALID" : "INVALID");

  Dump();

  return valid;
}


uint32_t WavHeader::GetSize() {
  return WAV_HEADER_SIZE;
}



///////////////////////////////////////////////////////////////////////////////
// class WavData
///////////////////////////////////////////////////////////////////////////////
WavData::WavData(): 
  buf(NULL), 
  bufLen(0) {}


bool WavData::Init(uint8_t *_buf, uint32_t _bufLen) {
  if (_bufLen <= 8) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Wav data size invalid. Expcted > 8, got %d\n", _bufLen);
    return false;
  }

  // A buffer for logging 4-character strings
  char scratchBuf[5];
  scratchBuf[4] = '\0'; // Ensure it's NULL-terminated

  if (memcmp(_buf, "data", 4) != 0) {
    memcpy(scratchBuf, _buf, 4);
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Data ID invalid. Expected %s, got %s\n", "data", scratchBuf);
    return false;
  }

  _buf += 4;
  uint32_t *dataSize = reinterpret_cast<uint32_t*>(_buf);
  if (*dataSize < _bufLen - 8) {
    // Something is wrong with the data...
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Wav data size parsed from buffer is invalid. Expcted >= %d, got %d\n", _bufLen, *dataSize);
    return false;
  }

  // The remaining portion of the buffer is expected to be samples.
  buf = _buf + 4;
  bufLen = _bufLen - 8;

  return true;
}


bool WavData::Update(uint8_t *_buf, uint32_t _bufLen) {
  buf = _buf;
  bufLen = _bufLen;
  return true;
}


} // namespace AudioLib
