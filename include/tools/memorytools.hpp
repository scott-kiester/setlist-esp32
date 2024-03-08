#ifndef __MEMORY_TOOLS_HPP___
#define __MEMORY_TOOLS_HPP___

#include <esp_heap_caps.h>

#include "log.hpp"


// Need to put some stuff in here to check for leaks and corruption. ESP32 has some tools I can use.

namespace Tools {

// cap is one of the MALLOC_CAP* defines in esp_heap_caps.h
void PrintHeapFreeSize(uint32_t comp, uint32_t sev, uint32_t cap);
void PrintAllHeapFreeSizes(uint32_t comp, uint32_t sev);

} // namespace Tools

#endif
