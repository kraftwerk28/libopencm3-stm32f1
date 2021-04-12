#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dbgmcu.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include <stdbool.h>

#include "utils.h"

/* MFRC522 onboard pinouts:
 * SDA - UART RX
 * SCK - UART DTRQ
 * MOSI - UART MX
 * MISO - UART TX */

static inline bool debugger_attached() { return (DBGMCU_CR & 0x07); }

int _write(int fd, char *ptr, int len) {
	(void)fd;
	int i = 0;
	for (; i < len && ptr[i]; i++) {
		itm_send_char(fd, ptr[i]);
	}
	return i;
}

static void setup_gpio() {
	nvic_enable_irq(NVIC_SYSTICK_IRQ);
	nvic_enable_irq(NVIC_EXTI0_IRQ);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GPIO6 | GPIO7);
	gpio_set(GPIOB, GPIO6);
	/* gpio_set(GPIOB, GPIO7); */

	exti_enable_request(EXTI0);
	exti_select_source(EXTI0, GPIOA);
	exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
}

void exti0_isr() {
	printf("EXTI0 interrupted\n");
	exti_reset_request(EXTI0);
	gpio_toggle(GPIOB, GPIO6);
}

static void setup_systick() {
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_interrupt_enable();
	systick_set_reload(rcc_ahb_frequency);
}

void sys_tick_handler() {}

static void setup_timers() {
	rcc_periph_clock_enable(RCC_TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);
	timer_set_prescaler(TIM2, (rcc_apb1_frequency / 1000) - 1);
	timer_set_period(TIM2, 2000 - 1);
	timer_enable_irq(TIM2, TIM_DIER_UIE);
}

/* void tim2_isr(void) { */
/* 	timer_clear_flag(TIM2, TIM_SR_UIF); */
/* 	return; */
/* } */

int main() {
	while (!debugger_attached()) {
		__asm("nop");
	}

	rcc_clock_setup_pll(&rcc_clock_config[RCC_CLOCK_VRANGE1_HSI_PLL_32MHZ]);
	setup_systick();
	/* setup_timers(); */
	setup_gpio();
	/* setup_temp_sensor(); */

	systick_counter_enable();
	/* timer_enable_counter(TIM2); */

	printf("AHB frequency = %dHz\n", rcc_ahb_frequency);

	/* uint8_t id[10]; */
	/* for (uint8_t i = 0; i < 255; i++) { */
	/* 	MFRC522_RandomId(id); */
	/* 	printf("%dth ID: ", i); */
	/* 	for (uint8_t j = 0; j < LEN(id); j++) { */
	/* 		printf("%02x ", id[j]); */
	/* 	} */
	/* 	printf("\n"); */
	/* } */

	while (1) {
		__asm("nop");
	}

	return 0;
}
