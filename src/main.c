#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "utils.h"

static const port_pin_t leds[] = {
    {GPIOC, GPIO13}, {GPIOA, GPIO0}, {GPIOA, GPIO1}, {GPIOA, GPIO2},
    {GPIOA, GPIO3},  {GPIOA, GPIO4}, {GPIOA, GPIO5}, {GPIOA, GPIO6},
    {GPIOB, GPIO0},  {GPIOB, GPIO1},
};

static const uint8_t nLeds = sizeof(leds) / sizeof(port_pin_t);
static uint8_t cur = 0;

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
  gpio_toggle(GPIOB, GPIO11);
}

int main() {
  setup_clocks();
  setup_systick();
  setup_timers();
  setup_gpio();

  while (1) {
    printf("INFO\n");
    wprintf("WARNING\n");
    eprintf("ERROR\n");
    for (uint32_t i = 0; i < rcc_ahb_frequency / 100; i++) {
    }
  }
  return 0;
}
