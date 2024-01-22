#ifndef __TSPOINT_HPP___
#define __TSPOINT_HPP___

#include <stdint.h>

class TSPoint {
public:
  TSPoint(): x(0), y(0), duration(0) {}
  TSPoint(uint16_t _x, uint16_t _y): x(_x), y(_y), duration(0) {}
  TSPoint(uint16_t _x, uint16_t _y, uint8_t _duration): x(_x), y(_y), duration(_duration) {}

  bool Equals(const TSPoint& thePoint) {
    return (thePoint.x == x && thePoint.y == y && thePoint.duration == duration);
  }

  uint16_t x;
  uint16_t y;
  uint8_t duration;
};

#endif
