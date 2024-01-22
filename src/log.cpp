#include <Arduino.h>

#include "log.hpp"

uint32_t logFlags = 0;
uint32_t logSeverity = 0;

uint32_t GetLogFlags() {
  return logFlags;
}

void SetLogFlags(uint32_t flags) {
  logFlags = flags;
}


void SetLogSeverity(LOG_SEVERITY sev) {
  logSeverity = sev;
}


#ifdef LOG_ENABLED

SemaphoreHandle_t logLock = NULL;

void logInit() {
  logLock = xSemaphoreCreateMutex();
}

void logLnImpl(const char *str) {
  xSemaphoreTake(logLock, portMAX_DELAY);
  Serial.println(str);
  xSemaphoreGive(logLock);
}

// A small and simple printf implementation for the serial console.
void logvPrintf(const char* str, ...) {

  va_list args;
  va_start(args, str); 

  xSemaphoreTake(logLock, portMAX_DELAY);

  for (const char *cur = str, *strEnd = str + strlen(str); cur < strEnd;) {
    // Avoid sending one character at a time
    char *segEnd = strchr(cur, '%');
    if (segEnd != NULL) {
      // Check for escape sequence, but make sure we're not at the start of the string first.
      if (segEnd > str && *(segEnd - 1) == '\\') {
        // Escaped. Skip it.
        Serial.write(cur, (segEnd - 1) - cur);
        cur = segEnd + 1;
        continue;
      }

      // Make sure there's something after the '%', and if there is then print it.
      if (segEnd + 1 >= strEnd) {
        Serial.println(F("*** INVALID logvPrintf ESCAPE SEQUENCE ***"));
        break;
      }

      // Print the stuff up to the format specifier.
      uint16_t len = segEnd - cur;
      Serial.write(cur, len);

      cur += len + 1; // Skip over the '%'

      char *writeThis = NULL;
      switch(*cur) {
        case 'd':
          Serial.print(va_arg(args, int));
          break;

        case 'u': 
          Serial.print(va_arg(args, unsigned));
          break;

        case 's':
          writeThis = va_arg(args, char*);
          Serial.write(writeThis, strlen(writeThis));
          break;

        default:
          Serial.print(F("*** INVALID logvPrintf format specifier: ")); Serial.println(*(cur + 1)); 
      }

      cur = segEnd + 2; // Skip the '%' and the format specifier letter

    } else {
      // End of string.
      Serial.write(cur, strEnd - cur);
      break;
    }
  }

  Serial.flush();
  xSemaphoreGive(logLock);

  va_end(args);
}


#endif // LOG_ENABLED
