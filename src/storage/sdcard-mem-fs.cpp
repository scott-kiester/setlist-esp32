#include "log.hpp"
#include "storage/sdcard.hpp"
#include "storage/sdcard-mem-fs.hpp"


#define MAX_MEMFS_FILE_SIZE (256 * 1024)



///////////////////////////////////////////////////////////////////////////////
// class SdCardMemFile
///////////////////////////////////////////////////////////////////////////////
SdCardMemFile::SdCardMemFile(FILE *theFile, const char *_fileName):
  fileBuf(NULL),
  fileBufLen(0),
  filePos(0),
  fileName(_fileName),
  valid(false)
{
  fseek(theFile, 0, SEEK_END);
  size_t fileSize = ftell(theFile);
  if (fileSize > MAX_MEMFS_FILE_SIZE) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardMemFile::SdCardMemFile: File exceeds max size of %d\n", MAX_MEMFS_FILE_SIZE);
    return;
  }

  fseek(theFile, 0, SEEK_SET);
  uint8_t *fileBuf = new uint8_t[fileSize];
  if (!fileBuf) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardMemFile::SdCardMemFile: Failed to allocate buffer of size %d\n", fileSize);
    return;
  }

  if (fread(fileBuf, fileSize, 1, theFile) < 1) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardMemFile::SdCardMemFile: Failed reading file into memory.\n");
    return;
  }

  fileBufLen = fileSize;
  valid = true;

  logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "SdCardMemFile::SdCardMemFile: Successfully read file of %d bytes into memory\n", fileSize);
}



SdCardMemFile::~SdCardMemFile() {
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardMemFile::~SdCardMemFile: Destructor called!\n");
  if (fileBuf) {
    delete fileBuf;
  }
}


size_t SdCardMemFile::read(uint8_t* buf, size_t size) {
  if (filePos + size > fileBufLen) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "SdCardMemFile::read: Attempt to read past the end of the file\n");
    return 0;
  }

  memcpy(buf, fileBuf, size);
  filePos += size;

  return size;
}


bool SdCardMemFile::seek(uint32_t pos, fs::SeekMode mode) {
  bool success = false;

  switch(mode) {
  case SEEK_CUR:
    if (filePos + pos <= fileBufLen) {
      filePos += pos;
      success = true;
    }
    break;

  case SEEK_SET:
    if (pos <= fileBufLen) {
      filePos = pos;
      success = true;
    }
    break;

  case SEEK_END:
    if (pos <= fileBufLen) {
      filePos = fileBufLen - pos;
      success = true;
    }
    break;
  }

  if (!success) {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "SdCardMemFile::seek failed\n");
  }

  return success;
}


SdCardMemFile::operator bool() {
  logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "SdCardMemFile::operator bool(): Returning %s\n", valid ? "TRUE" : "FALSE");
  return valid;
}


///////////////////////////////////////////////////////////////////////////////
// class SdCardMemFs
///////////////////////////////////////////////////////////////////////////////
fs::FileImplPtr SdCardMemFs::open(const char* path, const char* mode, const bool create) {
  if (create || !mode || mode[0] != 'r') {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardMemFs::open: Only read is supported\n");
    return NULL;
  }

  if (memFile) {
    if (fileName.size() > 0 && fileName.compare(path) != 0) {
      logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardMemFs::open: Attempt to open again with a different fileName. Original name: %s, new name: %s\n",
      fileName.c_str(),
      path);
      return NULL;
    }

    logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "SdCardMemFs::open: Returning cached memfile %s\n", path);

    return memFile;
  }

  // Open the file and read the contents into a buffer to be cached in SdCardMemFile
  FILE *theFile = fopen(path,"r");
  if (theFile) {
    std::shared_ptr<SdCardMemFile> tmpMemFile = std::make_shared<SdCardMemFile>(theFile, path);
    if (!tmpMemFile || !tmpMemFile->Valid()) {
      logPrintf(LOG_COMP_SDCARD, LOG_SEV_ERROR, "*** SdCardMemFile::open: Failed to create and initialize SdCardMemFile\n");
    }

    memFile = tmpMemFile;
    fclose(theFile);
  } else {
    logPrintf(LOG_COMP_SDCARD, LOG_SEV_WARN, "SdCardMemFs::open: Failed to open file %s\n", path);
  }

  logPrintf(LOG_COMP_SDCARD, LOG_SEV_VERBOSE, "SdCardMemFs::open: %s\n", memFile ? "SUCCESS" : "FAILURE");

  return memFile;
}


bool SdCardMemFs::exists(const char* path) {
  // stat(char*, struct stat*) doesn't seem to work on ESP32 (or maybe I just am not using it right).
  // Sledgehammer approach.
  bool exists = false;
  FILE *file = fopen(path, "r");
  if (file) {
    exists = true;
    fclose(file);
  } 

  logPrintf(LOG_COMP_SDCARD, LOG_SEV_INFO, "SdCardMemFs::exists: %s\n", exists ? "TRUE" : "FALSE");

  return exists;
}


const char * SdCardMemFs::mountpoint() {
  return SDCARD_ROOT;
}