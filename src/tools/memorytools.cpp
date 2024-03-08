#include "tools/memorytools.hpp"


namespace Tools {

class HeapCap {
public:
  uint32_t cap;
  const char *name;
};

// GCC doesn't like the bit-shift operations the MALLOC_CAP_* macros expand to when
// they're assigned to a uint32_t. However, the definition of MALLOC_CAP_INVALID makes
// it clear that Espressif meant for them to be a 32-bit values.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"

HeapCap heapCaps[] = {
  {MALLOC_CAP_EXEC, "Exec"},
  {MALLOC_CAP_32BIT, "32 Bit"},
  {MALLOC_CAP_8BIT, "8 Bit"},
  {MALLOC_CAP_DMA, "DMA"},
  {MALLOC_CAP_PID2, "PID2"},
  {MALLOC_CAP_PID3, "PID3"},
  {MALLOC_CAP_PID4, "PID4"},
  {MALLOC_CAP_PID5, "PID5"},
  {MALLOC_CAP_PID6, "PID6"},
  {MALLOC_CAP_PID7, "PID7"},
  {MALLOC_CAP_SPIRAM, "SPI RAM"},
  {MALLOC_CAP_INTERNAL, "Internal"},
  {MALLOC_CAP_DEFAULT, "Default"},
  {MALLOC_CAP_IRAM_8BIT, "8 Bit IRAM"},
  {MALLOC_CAP_RETENTION, "Retention (DMA)"},
  {MALLOC_CAP_RTCRAM, "RTC RAM"},
  {MALLOC_CAP_INVALID, "Invalid"}, // Must always be last
};

#pragma GCC diagnostic pop


void printHeapFreeSize(uint32_t comp, uint32_t sev, HeapCap& heapCap, bool printingAll) {
  size_t value = heap_caps_get_free_size(heapCap.cap);
  if (!printingAll) {
    logPrintf(comp, sev, "Heap free size for %s: %z\n", heapCap.name, value);
  } else {
    logPrintf(comp, sev, "\t%s:\t%z\n", heapCap.name, value);
  }
}


void PrintHeapFreeSize(uint32_t comp, uint32_t sev, uint32_t cap) {  
  bool found = false;
  for (uint8_t i = 0; heapCaps[i].cap != MALLOC_CAP_INVALID; i++) {
    if (heapCaps[i].cap == cap) {
      printHeapFreeSize(comp, sev, heapCaps[i], false);
      found = true;
      break;
    }
  }

  if (!found) {
    logPrintf(comp, sev, "PrintHeapFreeSize: Unknown heap cap: 0x%x\n", cap);
  }
}


void PrintAllHeapFreeSizes(uint32_t comp, uint32_t sev) {
  logPrintf(comp, sev, "\n**********\nDumping free memory from all heaps:\n\n");

  for (uint8_t i = 0; heapCaps[i].cap != MALLOC_CAP_INVALID; i++) {
    printHeapFreeSize(comp, sev, heapCaps[i], true);
  }

  logPrintf(comp, sev, "\n**********\n\n")
}


} // namespace Tools
