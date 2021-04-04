#ifndef __UTILS_H__
#define __UTILS_H__

#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/itm.h>
#include <libopencm3/cm3/memorymap.h>
#include <libopencm3/cm3/scs.h>
#include <libopencm3/cm3/tpiu.h>
#include <stdint.h>
#include <stdio.h>

#define printf(...) (dprintf(0, __VA_ARGS__))
#define wprintf(...) (dprintf(1, __VA_ARGS__))
#define eprintf(...) (dprintf(2, __VA_ARGS__))

typedef struct {
  uint32_t port;
  uint16_t pin;
} port_pin_t;

float mix(float x, float a, float b, float c, float d) {
  return (x * d - x * c - a * d + b * c) / (b - a);
}

uint8_t itm_send_char(uint32_t channel, uint8_t ch) {
  if (((ITM_TCR & ITM_TCR_ITMENA) != 0UL) && ((*ITM_TER & 1UL) != 0UL)) {
    while (ITM_STIM32(0) == 0) {
    }
    ITM_STIM8(channel) = ch;
  }
  return ch;
}

int _write(int fd, char *ptr, int len) {
  (void)fd;
  int i = 0;
  for (; i < len && ptr[i]; i++) {
    itm_send_char(fd, ptr[i]);
  }
  return i;
}

#endif
