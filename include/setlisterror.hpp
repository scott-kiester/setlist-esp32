#ifndef __SETLIST_ERROR_HPP___
#define __SETLIST_ERROR_HPP___

enum SetlistError {
  SL_ERR_OK = 0,
  SL_ERR_NO_NETWORK,
  SL_ERR_HTTP_CLIENT,
  SL_ERR_HTTP_SERVER,
  SL_ERR_INSUFFICIENT_MEMORY,
  SL_ERR_READ_HTTP_RESPONSE_FAILED,
  SL_ERR_FILE_CREATE_FAILED,
  SL_ERR_FILE_WRITE,
  SL_ERR_BAD_CONFIG,
  SL_ERR_LAST, // Add new errors above here. This must always be last.
};

const char* SetlistErrorToString(SetlistError err);

#endif
