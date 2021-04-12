#pragma once

#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/itm.h>
#include <libopencm3/cm3/memorymap.h>
#include <libopencm3/cm3/scs.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/tpiu.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <stdint.h>
#include <stdio.h>

// TODO: shall be included from <stdio.h>
extern int dprintf(int fd, const char *restrict format, ...);

#define printf(...) (dprintf(0, __VA_ARGS__))
#define wprintf(...) (dprintf(1, __VA_ARGS__))
#define eprintf(...) (dprintf(2, __VA_ARGS__))
#define LEN(array) (sizeof(array) / sizeof(array[0]))

typedef struct {
	uint32_t port;
	uint16_t pin;
} port_pin_t;

static inline float mix(float x, float a, float b, float c, float d) {
	return (x * d - x * c - a * d + b * c) / (b - a);
}

/* SysTick based delay */
static inline void delay(uint32_t ms) {
	uint32_t n = (rcc_ahb_frequency * 0.0005) * ms;
	while (n--) {
	}
}

static inline uint8_t spi_transfer(uint32_t spi, uint8_t data) {
	spi_send(spi, data);
	return spi_read(spi);
}
