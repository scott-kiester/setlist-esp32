#include <stdlib.h>

#include "audio/memwav.hpp"
#include "log.hpp"
#include "storage/sdcard-fs.hpp"


namespace AudioLib {

#define MAX_MEMWAV_FILE_SIZE (256 * 1024)

// WAV aka Resource Interchange File Format (RIFF): https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Docs/riffmci.pdf



///////////////////////////////////////////////////////////////////////////////
// class MemWav
///////////////////////////////////////////////////////////////////////////////
MemWav::MemWav():
  wavBufLen(0),
  valid(false) {}


MemWav::~MemWav() {
}


bool MemWav::readFromFile(const char *fileName) {
  FILE *theFile = fopen(fileName,"r");
  if (!theFile) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** MemWav::readFromFile: Unable to open .wav file for reading. File: %s\n", fileName);
    return false;
  }

  bool success = false;

  fseek(theFile, 0, SEEK_END);
  size_t fileSize = ftell(theFile);
  if (fileSize <= MAX_MEMWAV_FILE_SIZE) {
      fseek(theFile, 0, SEEK_SET);
      wavBuf = std::unique_ptr<uint8_t[]>(new uint8_t[fileSize]);
      if (wavBuf) {
        if (fread(wavBuf.get(), fileSize, 1, theFile) == 1) {
          logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "MemWav::readFromFile: Read contents of file %s\n", fileName);
          wavBufLen = fileSize;
          success = true;
        } else {
          logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** MemWav::readFromFile: Failed reading file into memory.\n");
        }
      } else {
        logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** MemWav::readFromFile: Failed to allocate buffer of %d bytes\n", fileSize);
      }
  } else {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** MemWav::readFromFile: File exceeds max size of %d\n", MAX_MEMWAV_FILE_SIZE);
  }

  return success;
}


bool MemWav::InitFromFile(const char *fileName) {
  if (!readFromFile(fileName)) {
    return false;
  }

  if (!wavHeader.ReadFromBuffer(wavBuf.get(), wavBufLen)) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "Failed to parse .wav header from file %s\n", fileName);
    return false;
  }

  uint32_t headerSize = wavHeader.GetSize();
  if (wavBufLen <= headerSize) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "WAV file contains no samples! Header size: %d, total size: %d, file: %s\n", headerSize, wavBufLen, fileName);
    return false;
  }

  uint8_t *dataStart = wavBuf.get() + headerSize;
  uint32_t dataLen = wavBufLen - headerSize;
  if (!wavData.Init(dataStart, dataLen)) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "WAV data invalid. File %s\n", fileName);
    return false;
  }

  samples.samples = wavData.GetData();
  samples.len = wavData.GetDataLen();

  valid = true;
  return true;
}


uint32_t MemWav::GetSampleRate() {
  return wavHeader.samplesPerSecond;
}


uint16_t MemWav::GetBitsPerSample() {
  return wavHeader.bitsPerSample;
}


const AudioSamples* MemWav::GetSamples() {
  if (valid) {
    return &samples;
  } else {
    return NULL;
  }
}


} // namespace AudioLib
