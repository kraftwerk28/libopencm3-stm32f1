#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/dbgmcu.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include "mfrc522.h"
#include "utils.h"

/*
 * SDA - UART RX
 * SCK - UART DTRQ
 * MOSI - UART MX
 * MISO - UART TX
 */

#define debugger_attached() (DBGMCU_CR & 0x07)

int _write(int fd, char *ptr, int len) {
	(void)fd;
	int i = 0;
	for (; i < len && ptr[i]; i++) {
		itm_send_char(fd, ptr[i]);
	}
	return i;
}

static const port_pin_t leds[] = {
	{GPIOC, GPIO13}, {GPIOA, GPIO0}, {GPIOA, GPIO1}, {GPIOA, GPIO2},
	{GPIOA, GPIO3},	 {GPIOA, GPIO4}, {GPIOA, GPIO5}, {GPIOA, GPIO6},
	{GPIOB, GPIO0},	 {GPIOB, GPIO1},
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

/* static uint32_t systick_counter = 0; */

void sys_tick_handler() {
	/* systick_counter = (systick_counter + 1) % 1000; */
	/* if (!systick_counter) { */
	/* 	update_leds(); */
	/* } */
}

static void setup_clocks() {
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void setup_gpio() {
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	/* rcc_periph_clock_enable(RCC_GPIOC); */

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
				  GPIO_SPI1_NSS);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
				  GPIO_SPI1_SCK | GPIO_SPI1_MOSI);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
				  GPIO_SPI1_MISO);
	gpio_set(GPIOA, GPIO_SPI1_NSS);

	/* gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, */
	/* 			  GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6); */
	// Disabled some of them to let SPI run
	/* gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, */
	/* 			  GPIO0 | GPIO1 | GPIO2 | GPIO3); */
	/* gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, */
	/* 			  GPIO0 | GPIO1 | GPIO11); */
	/* gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, */
	/* 			  GPIO13); */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
				  GPIO11);
	gpio_set(GPIOB, GPIO11); // Resetting red led
}

static void setup_systick() {
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(0xffffff);
}

static void setup_timers() {
	rcc_periph_clock_enable(RCC_TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);
	timer_set_prescaler(TIM2, (rcc_apb1_frequency / 1000) - 1);
	timer_set_period(TIM2, 2000 - 1);
	timer_enable_irq(TIM2, TIM_DIER_UIE);
}

static void setup_usart() {
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
				  GPIO_USART1_TX | GPIO_USART1_RX);
	usart_set_baudrate(USART1, 9600);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_enable(USART1);
}

static void setup_spi() {
	rcc_periph_clock_enable(RCC_SPI1);
	spi_reset(SPI1);
	spi_init_master(
		SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_128, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_enable_software_slave_management(SPI1);
	spi_set_nss_high(SPI1);
	spi_enable(SPI1);
}

void tim2_isr(void) { timer_clear_flag(TIM2, TIM_SR_UIF); }

void print_version() {
	uint8_t ver = MFRC522_ReadCharFromReg(VersionReg);
	printf("version code: %#x\n", ver);
}

int main() {
	setup_clocks();
	setup_systick();
	setup_timers();
	setup_gpio();
	setup_spi();

	systick_counter_enable();
	timer_enable_counter(TIM2);

	while (!debugger_attached()) {
		delay(150);
		gpio_toggle(GPIOB, GPIO11);
	}

	gpio_set(GPIOB, GPIO11);

	printf("-------------------------------------------------------------------"
		   "-------------\n");
	printf("AHB frequency = %dHz\n", rcc_ahb_frequency);

	print_version();
	MFRC522_SelfTest();

	while (1) {
		delay(150);
		print_version();
		/* printf("timer: %d, %d\n", MFRC522_ReadCharFromReg(TCounterValReg1),
		 */
		/* 	   MFRC522_ReadCharFromReg(TCounterValReg1)); */
		/* wprintf("AHB frequency: %d\n", rcc_ahb_frequency); */
		/* eprintf("APB1 frequency: %d\n", rcc_apb1_frequency); */
		/* printf("APB2 frequency: %d\n", rcc_apb2_frequency); */
		/* for (uint32_t i = 0; i < rcc_ahb_frequency / 1000; i++) { */
		/* } */
	}

	return 0;
}
