#include "debugchecks.hpp"
#include "log.hpp"
#include "storage/sdcard.hpp"
#include "storage/sdcard-fs.hpp"


///////////////////////////////////////////////////////////////////////////////
// class SdCardFile
///////////////////////////////////////////////////////////////////////////////
SdCardFile::SdCardFile(FILE *theFile, const char *strFilename):
  file(theFile),
  fileName(strFilename),
  statsCached(false) {}


SdCardFile::~SdCardFile() {
  if (file) {
    fclose(file);
  }
}



size_t SdCardFile::write(const uint8_t *buf, size_t size) {
  return fwrite(buf, size, 1, file);
}


size_t SdCardFile::read(uint8_t* buf, size_t size) {
  return fread(buf, size, 1, file);
}


void SdCardFile::flush() {
  fflush(file);
}


bool SdCardFile::seek(uint32_t pos, fs::SeekMode mode) {
  return (fseek(file, pos, mode) != 0);
}


size_t SdCardFile::position() const {
  return ftell(file);
}


size_t SdCardFile::size() const {
  size_t theSize = 0;

  // This is ugly, but we're only adding cached data. There's probably a more elegant solution,
  // but this works.
  if (const_cast<SdCardFile*>(this)->doStat()) {
    theSize = stats.st_size;
  }

  return theSize;
}


// Do we actually need to do anything here?
bool SdCardFile::setBufferSize(size_t size) { return true; }


void SdCardFile::close() {
  if(file) {
    fclose(file);
    file = NULL;
  }
}


time_t SdCardFile::getLastWrite() {
  time_t lastWrite = 0;

  if (const_cast<SdCardFile*>(this)->doStat()) {
    lastWrite = stats.st_mtim.tv_sec;
  }

  return lastWrite;
}


const char* SdCardFile::path() const {
  // Not implemented
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "*** SdCardFile::path() NOT IMPLEMENTED.\n");
  DC_ASSERT(false);
  return NULL;
}


const char* SdCardFile::name() const {
  // This is probably wrong for files that were opened with a directory in the path. Probably
  // won't matter...
  static bool warned = false;
  if (!warned) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "SdCardFile::name() returning %s. Ensure it's correct.\n", fileName.c_str());
    warned = true;
  }

  return fileName.c_str();
}


boolean SdCardFile::isDirectory(void) {
  boolean isDir = false;

  if (const_cast<SdCardFile*>(this)->doStat()) {
    isDir = !S_ISREG(stats.st_mode);
  }

  return isDir;
}


fs::FileImplPtr SdCardFile::openNextFile(const char* mode) {
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "*** SdCardFile::openNextFile() NOT IMPLEMENTED.\n");
  return NULL;
}


boolean SdCardFile::seekDir(long position) {
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "*** SdCardFile::seekDir() NOT IMPLEMENTED.\n");
  return false;
}

String SdCardFile::getNextFileName(void) {
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "*** SdCardFile::getNextFileName() NOT IMPLEMENTED.\n");
  return "";
}


String SdCardFile::getNextFileName(bool *isDir) {
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "*** SdCardFile::getNextFileName(bool*) NOT IMPLEMENTED.\n");
  return "";
}


void SdCardFile::rewindDirectory(void) {
  fseek(file, 0, SEEK_SET);
}


SdCardFile::operator bool() {
  return file != NULL;
}


bool SdCardFile::doStat() {
  if (!statsCached && fstat(fileno(file), &stats) == 0) {
    statsCached = true;
  } else {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "SdCardFile: Unable to stat file\n");
  }

  return statsCached;
}


///////////////////////////////////////////////////////////////////////////////
// class SdCardFs
///////////////////////////////////////////////////////////////////////////////
fs::FileImplPtr SdCardFs::open(const char* path, const char* mode, const bool create) {
  std::string posixMode = mode;
  if (!create && posixMode.compare(FILE_WRITE) == 0) {
    posixMode += '+';
  }

  logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "SdCardFs::open: path: %s, mode: %s, origMode: %s\n", path, posixMode.c_str(), mode);

  SdCardFile *fileObj = NULL;
  FILE *theFile = fopen(path, posixMode.c_str());
  if (theFile) {
    fileObj = new SdCardFile(theFile, path);
    if (!fileObj) {
      fclose(theFile);
      logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardFs::open: Failed to allocate new SdCardFile\n");
    }
  } else {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "SdCardFs::open: Failed to open file %s\n", path);
  }

  return fs::FileImplPtr(fileObj);
}


bool SdCardFs::exists(const char* path) {
  // stat(char*, struct stat*) doesn't seem to work on ESP32 (or maybe I just am not using it right).
  // Sledgehammer approach.
  bool exists = false;
  FILE *file = fopen(path, "r");
  if (file) {
    exists = true;
    fclose(file);
  }

  return exists;
}


bool SdCardFs::rename(const char* pathFrom, const char* pathTo) {
  return (::rename(pathFrom, pathTo) == 0);
}


bool SdCardFs::remove(const char* path) {
  return (::remove(path) == 0);
}


bool SdCardFs::mkdir(const char *path) {
  return (::mkdir(path, 777) == 0);
}


bool SdCardFs::rmdir(const char *path) {
  return (::rmdir(path) == 0);
}


void mountpoint(const char *) {
  // Not implemented. Probably needs to be if it ever gets called.
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardFile::configEvent(const char*) NOT IMPLEMENTED.\n");
  DC_ASSERT(false);
}


const char* mountpoint() {
  return SDCARD_ROOT;
}
