#include "log.hpp"
#include "setlisterror.hpp"

class SetlistErrorData {
public:
  SetlistError err;
  const char *message;
};

// This array MUST be in the same order as the SetListError enum in setlisterror.hpp
const SetlistErrorData setlistErrorTable[] = {
  {SL_ERR_OK, "The operation was successful."},
  {SL_ERR_NO_NETWORK, "Not connected to a network."},
  {SL_ERR_HTTP_CLIENT, "The HTTP Client encountered an error."},
  {SL_ERR_HTTP_SERVER, "The HTTP Server returned an error."},
  {SL_ERR_INSUFFICIENT_MEMORY, "Out of memory."},
  {SL_ERR_READ_HTTP_RESPONSE_FAILED, "Failed to read the HTTP response."},
  {SL_ERR_FILE_CREATE_FAILED, "File creation failed."},
  {SL_ERR_FILE_WRITE, "File write operation failed."},
  {SL_ERR_BAD_CONFIG, "One or more of the config files could not be processed. Check the syntax."},
  {SL_ERR_LAST, ""},
};

const char* SetlistErrorToString(SetlistError err) {
  if (err < 0 || err > SL_ERR_LAST) {
    logPrintf(LOG_COMP_GENERAL, LOG_SEV_ERROR, "SetlistErrorToString called with invalid error code: %d\n", err);
    return "";
  }

  return setlistErrorTable[err].message;
}