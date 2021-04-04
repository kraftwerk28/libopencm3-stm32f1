#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

typedef struct {
  uint32_t port;
  uint16_t pin;
} port_pin_t;

float mix(float x, float a, float b, float c, float d) {
  return (x * d - x * c - a * d + b * c) / (b - a);
}

#endif
