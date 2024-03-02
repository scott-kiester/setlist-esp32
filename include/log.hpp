#ifndef __LOG_HPP___
#define __LOG_HPP___

#include <Arduino.h>
#include <stdint.h>

#define LOG_ENABLED 1

#define LOG_COMP_ALL        0xffffffff

#define LOG_COMP_DEFAULT    0x00000000
#define LOG_COMP_COMPONENT  0x00000001
#define LOG_COMP_BUTTON     0x00000002
#define LOG_COMP_POINTS     0x00000004
#define LOG_COMP_DEBUG      0x00000010
#define LOG_COMP_SDCARD     0x00000020
#define LOG_COMP_GRID       0x00000040
#define LOG_COMP_SONG       0x00000100
#define LOG_COMP_SCREEN     0x00000200
#define LOG_COMP_BLUETOOTH  0x00000400
#define LOG_COMP_GENERAL    0x00001000
#define LOG_COMP_AUDIO      0x00002000
#define LOG_COMP_SERIALIZE  0x00004000
#define LOG_COMP_TRIGGER    0x00010000
#define LOG_COMP_NET        0x00020000

enum LOG_SEVERITY {
  LOG_SEV_ERROR = 1,
  LOG_SEV_WARN,
  LOG_SEV_INFO,
  LOG_SEV_DEBUG,
  LOG_SEV_VERBOSE
};

// Well, this is evil... but it's fast.
extern uint32_t logFlags;
extern uint32_t logSeverity;

#define IS_LOG_ENABLED(x, sev) (LOG_ENABLED && (logFlags & x && logSeverity <= sev))

void logInit();

#ifdef LOG_ENABLED
void logvPrintf(const char* str, ...);
void logLnImpl(const char* str);

#define logLn(comp, sev, x) if ((logFlags & comp) && logSeverity >= sev) { logLnImpl(x); }
#define logPrintf(comp, sev, str, ...) if (logFlags & (comp) && logSeverity >= (sev)) { logvPrintf(str, ##__VA_ARGS__); }

//void logPointInternal(TSPoint* p);
//#define logPoint(comp, sev, p) if (logFlags & comp && logSeverity <= sev) { logPointInternal(p); }

#else
//#define log(comp, x)
#define logLn(comp, sev, x)
#define logPoint(comp, sev, x)
#define logPrintf(comp, sev, str, ...)

#endif

#endif
