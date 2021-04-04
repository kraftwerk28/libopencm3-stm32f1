#include <libopencm3/cm3/itm.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scs.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/tpiu.h>
#include <libopencm3/stm32/dbgmcu.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "utils.h"

static const port_pin_t leds[] = {
    {GPIOC, GPIO13}, {GPIOA, GPIO0}, {GPIOA, GPIO1}, {GPIOA, GPIO2},
    {GPIOA, GPIO3},  {GPIOA, GPIO4}, {GPIOA, GPIO5}, {GPIOA, GPIO6},
    {GPIOB, GPIO0},  {GPIOB, GPIO1},
};

static const uint8_t nLeds = sizeof(leds) / sizeof(port_pin_t);
static uint8_t cur = 0;

// From
// https://github.com/libopencm3/libopencm3-examples/blob/master/examples/stm32/f1/stm32-h103/traceswo/traceswo.c#L36-L59
static void trace_setup(void) {
  /* Enable trace subsystem (we'll use ITM and TPIU). */
  SCS_DEMCR |= SCS_DEMCR_TRCENA;

  /* Use Manchester code for asynchronous transmission. */
  TPIU_SPPR = TPIU_SPPR_ASYNC_MANCHESTER;
  TPIU_ACPR = 7;

  /* Formatter and flush control. */
  TPIU_FFCR &= ~TPIU_FFCR_ENFCONT;

  /* Enable TRACESWO pin for async mode. */
  DBGMCU_CR = DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_MODE_ASYNC;

  /* Unlock access to ITM registers. */
  /* FIXME: Magic numbers... Is this Cortex-M3 generic? */
  *((volatile uint32_t *)0xE0000FB0) = 0xC5ACCE55;

  /* Enable ITM with ID = 1. */
  ITM_TCR = (1 << 16) | ITM_TCR_ITMENA;
  /* Enable stimulus port 1. */
  ITM_TER[0] = 1;
}

static void trace_send(char c) {
  while (!(ITM_STIM8(0) & ITM_STIM_FIFOREADY))
    ;
  ITM_STIM8(c);
}

int _write(int fd, char *ptr, int len) {
  gpio_toggle(GPIOB, GPIO11);
  (void)fd;
  int i = 0;
  for (; i < len && ptr[i]; i++) {
    trace_send(ptr[i]);
  }
  return i;
}

static void update_leds() {
  for (uint8_t i = 0; i < nLeds; i++) {
    if ((cur >> i) & 1) {
      gpio_set(leds[i].port, leds[i].pin);
    } else {
      gpio_clear(leds[i].port, leds[i].pin);
    }
  }
  cur = (cur + 1) & 0x03ff;
}

static uint32_t systick_counter = 0;

void sys_tick_handler() {
  systick_counter = (systick_counter + 1) % 1000;
  if (!systick_counter) {
    update_leds();
  }
}

static void setup_clocks() {
  rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void setup_gpio() {
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO0 | GPIO1 | GPIO11);
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO13);
  gpio_set(GPIOB, GPIO11); // Resetting red led
}

static void setup_systick() {
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_interrupt_enable();
  systick_set_reload(rcc_ahb_frequency / 1000 - 1);
  systick_counter_enable();
}

static void setup_timers() {
  rcc_periph_clock_enable(RCC_TIM2);
  nvic_enable_irq(NVIC_TIM2_IRQ);
  timer_set_prescaler(TIM2, (rcc_apb1_frequency / 1000) - 1);
  timer_set_period(TIM2, 2000 - 1);
  timer_enable_counter(TIM2);
  timer_enable_irq(TIM2, TIM_DIER_UIE);
}

void tim2_isr(void) {
  timer_clear_flag(TIM2, TIM_SR_UIF);
  /* gpio_toggle(GPIOB, GPIO11); */
}

int main() {
  setup_clocks();
  setup_systick();
  setup_timers();
  setup_gpio();
  trace_setup();

  while (1) {
    printf("%s\n", "Hello, world!");
    for (uint32_t i = 0; i < rcc_ahb_frequency / 100; i++) {
      __asm("nop");
    }
  }

  return 0;
}
