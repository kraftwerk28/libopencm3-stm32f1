#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <stdint.h>

typedef struct {
  uint32_t port;
  uint16_t pin;
} led_t;

static const led_t leds[10] = {
    {GPIOC, GPIO13}, {GPIOA, GPIO0}, {GPIOA, GPIO1}, {GPIOA, GPIO2},
    {GPIOA, GPIO3},  {GPIOA, GPIO4}, {GPIOA, GPIO5}, {GPIOA, GPIO6},
    {GPIOB, GPIO0},  {GPIOB, GPIO1},
};

static const int nLeds = sizeof(leds) / sizeof(led_t);
static int cur = 0;

void sys_tick_handler() {
  for (int i = 0; i < nLeds; i++) {
    if ((cur >> i) & 1) {
      gpio_set(leds[i].port, leds[i].pin);
    } else {
      gpio_clear(leds[i].port, leds[i].pin);
    }
  }
  cur = (cur + 1) & 0x03ff;
}

int main() {
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSI_24MHZ]);
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO0 | GPIO1 | GPIO11);
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO13);
  gpio_set(GPIOB, GPIO11);

  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_interrupt_enable();
  systick_set_reload(24000000 - 1);
  systick_counter_enable();

  while (1)
    ;

  return 0;
}
