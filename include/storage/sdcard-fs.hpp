#ifndef __FS_HPP___
#define __FS_HPP___

#include <string>

#include <FS.h>
#include <FSImpl.h>

class SdCardFile : public fs::FileImpl {
public:
  SdCardFile(FILE *theFile, const char* strFilename);
  virtual ~SdCardFile();

  virtual size_t write(const uint8_t *buf, size_t size);
  virtual size_t read(uint8_t* buf, size_t size);
  virtual void flush();
  virtual bool seek(uint32_t pos, fs::SeekMode mode);
  virtual size_t position() const;
  virtual size_t size() const;
  virtual bool setBufferSize(size_t size);
  virtual void close();
  virtual time_t getLastWrite();
  virtual const char* path() const;
  virtual const char* name() const;
  virtual boolean isDirectory(void);
  virtual fs::FileImplPtr openNextFile(const char* mode);
  virtual boolean seekDir(long position);
  virtual String getNextFileName(void);
  virtual String getNextFileName(bool *isDir);
  virtual void rewindDirectory(void);
  virtual operator bool();

private:
  bool doStat();

  FILE *file;
  std::string fileName;

  bool statsCached;
  struct stat stats;
};


class SdCardFs : public fs::FSImpl {
public:
  SdCardFs() {}
  virtual ~SdCardFs() {}

  virtual fs::FileImplPtr open(const char* path, const char* mode, const bool create);
  virtual bool exists(const char* path);
  virtual bool rename(const char* pathFrom, const char* pathTo);
  virtual bool remove(const char* path);
  virtual bool mkdir(const char *path);
  virtual bool rmdir(const char *path);
  void mountpoint(const char *);
  const char * mountpoint();
};

#endif