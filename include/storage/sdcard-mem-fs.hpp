#ifndef __SDCARD_MEM_FS_HPP___
#define __SDCARD_MEM_FS_HPP___

#include <string>

#include <FS.h>
#include <FSImpl.h>


// Reads a file and keeps it in memory. This is used for the click samples, since the audio
// library doesn't provide an API for it.
class SdCardMemFile : public fs::FileImpl {
public:
  SdCardMemFile(FILE *theFile, const char *fileName);
  virtual ~SdCardMemFile();

  virtual size_t write(const uint8_t *buf, size_t size) { return 0; }
  virtual size_t read(uint8_t* buf, size_t size);
  virtual void flush() {}
  virtual bool seek(uint32_t pos, fs::SeekMode mode);
  virtual size_t position() const { return filePos; }
  virtual size_t size() const { return fileBufLen; }
  virtual bool setBufferSize(size_t size) { return true; }
  virtual void close() {}
  virtual time_t getLastWrite() { return 0; }
  virtual const char* path() const { return NULL; }
  virtual const char* name() const { return fileName.c_str(); }
  virtual boolean isDirectory(void) { return false; }
  virtual fs::FileImplPtr openNextFile(const char* mode) { return NULL; }
  virtual boolean seekDir(long position) { return false; }
  virtual String getNextFileName(void) { return ""; }
  virtual String getNextFileName(bool *isDir) { return ""; }
  virtual void rewindDirectory(void) {}
  virtual operator bool();

  bool Valid() { return valid; }

private:
  uint8_t *fileBuf;
  uint32_t fileBufLen;
  uint32_t filePos;

  std::string fileName;

  bool valid;
};


class SdCardMemFs : public fs::FSImpl {
public:
  SdCardMemFs() {}
  virtual ~SdCardMemFs() {}

  virtual fs::FileImplPtr open(const char* path, const char* mode, const bool create);
  virtual bool exists(const char* path);
  virtual bool rename(const char* pathFrom, const char* pathTo) { return false; }
  virtual bool remove(const char* path) { return false; }
  virtual bool mkdir(const char *path) { return false; }
  virtual bool rmdir(const char *path) { return false; }
  void mountpoint(const char *) {}
  const char * mountpoint();

private:
  fs::FileImplPtr memFile;
  std::string fileName;
};


#endif
