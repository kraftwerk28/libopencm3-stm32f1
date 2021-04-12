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

#include "mfrc522.h"
#include "utils.h"

/* MFRC522 onboard pinouts:
 * SDA - UART RX
 * SCK - UART DTRQ
 * MOSI - UART MX
 * MISO - UART TX */

/* if (((ITM_TCR & ITM_TCR_ITMENA) != 0UL) && ((*ITM_TER & 1UL) != 0UL)) {
 */
/* 	while (!(ITM_STIM32(0) & ITM_STIM_FIFOREADY)) { */
/* 	} */
/* 	ITM_STIM8(channel) = ch; */
/* } */

static inline bool debugger_attached() { return (DBGMCU_CR & 0x07); }

static inline uint8_t itm_send_char(uint32_t channel, uint8_t ch) {
	while (!(ITM_STIM8(0) & ITM_STIM_FIFOREADY)) {
	}
	ITM_STIM8(channel) = ch;
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

static const port_pin_t leds[] = {
	{GPIOC, GPIO13}, {GPIOA, GPIO0}, {GPIOA, GPIO1}, {GPIOA, GPIO2},
	{GPIOA, GPIO3},	 {GPIOA, GPIO4}, {GPIOA, GPIO5}, {GPIOA, GPIO6},
	{GPIOB, GPIO0},	 {GPIOB, GPIO1},
};

static uint8_t cur = 0;
static void update_leds() {
	for (uint8_t i = 0; i < LEN(leds); i++) {
		if ((cur >> i) & 1) {
			gpio_set(leds[i].port, leds[i].pin);
		} else {
			gpio_clear(leds[i].port, leds[i].pin);
		}
	}
	cur = (cur + 1) & 0x03ff;
}

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
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_AFIO);

	/* SPI1 Slave Select pin */
	/* gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
	 */
	/* 			  GPIO_SPI1_NSS); */

	/* MFRC522 IRQ pin */
	/* gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0);
	 */

	/* SPI1 Clock and MOSI pins */
	/* gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, */
	/* 			  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, */
	/* 			  GPIO_SPI1_SCK | GPIO_SPI1_MOSI); */

	/* SPI1 MISO pin */
	/* gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, */
	/* 			  GPIO_SPI1_MISO); */

	/* Green LED */
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
				  GPIO13);

	/* On-board LED */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
				  GPIO11);

	/* Echo pin */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
				  GPIO_TIM2_CH2);

	nvic_enable_irq(NVIC_EXTI1_IRQ);
	exti_select_source(EXTI1, GPIOA);
	exti_set_trigger(EXTI1, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI1);

	/* Trigger pin */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM3_CH1);

	/* Slave select must be 1 by default */
	gpio_set(GPIOA, GPIO_SPI1_NSS);
	/* gpio_set(GPIOC, GPIO13); */
	/* Turn off on-board led */
	gpio_set(GPIOB, GPIO11);

	/* gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, */
	/* 			  GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6); */
	// Disabled some of them to let SPI run
	/* gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, */
	/* 			  GPIO0 | GPIO1 | GPIO2 | GPIO3); */
	/* gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, */
	/* 			  GPIO0 | GPIO1 | GPIO11); */
	/* gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, */
	/* 			  GPIO13); */
}

void exti1_isr() {
	exti_reset_request(EXTI1);
	if (gpio_get(GPIOA, GPIO1)) {
		timer_set_counter(TIM2, 0);
	} else {
		uint32_t distance = timer_get_counter(TIM2) / 58;
		printf("Distance: %d cm.\n", distance);
	}
}

static void setup_systick() {
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(0xffffff);
}

static void setup_timers() {
	/* Ultrasonic echo timer setup */
	rcc_periph_clock_enable(RCC_TIM2);
	/* nvic_enable_irq(NVIC_TIM2_IRQ); */
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(TIM2, (rcc_apb1_frequency * 0.000001 * 2) - 1);
	timer_set_period(TIM2, 0xffff - 1);
	/* timer_ic_set_input(TIM2, TIM_IC2, TIM_IC_IN_TI1); */
	/* timer_enable_irq(TIM2, TIM_DIER_UIE); */
	/* timer_ic_enable(TIM2, TIM_IC2); */

	/* Ultrasonic trigger timer setup */
	rcc_periph_clock_enable(RCC_TIM3);
	timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(TIM3, (rcc_apb1_frequency * 0.000001) - 1);
	timer_set_period(TIM3, 0xffff - 1);
	timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM1);
	timer_set_oc_value(TIM3, TIM_OC1, 10 - 1);
	timer_enable_oc_output(TIM3, TIM_OC1);
}

void tim2_isr(void) {
	if (timer_get_flag(TIM2, TIM_SR_CC2IF)) {
		timer_clear_flag(TIM2, TIM_SR_CC2IF);
		printf("INPUT CAPTURE interrupt; tim2 = %u/2000\n",
			   timer_get_counter(TIM2));
	}
	/* if (timer_get_flag(TIM2, TIM_SR_TIF)) { */
	/* 	timer_clear_flag(TIM2, TIM_SR_TIF); */
	/* 	printf("TRIGGER trigger interrupt; tim2 = %u/2000\n", */
	/* 		   timer_get_counter(TIM2)); */
	/* } */
	/* if (timer_get_flag(TIM2, TIM_SR_UIF)) { */
	/* 	timer_clear_flag(TIM2, TIM_SR_UIF); */
	/* 	printf("UPDATE interrupt; tim2 = %u/2000\n", timer_get_counter(TIM2));
	 */
	/* } */
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
		SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_32, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_enable_software_slave_management(SPI1);
	spi_set_nss_high(SPI1);
	spi_enable(SPI1);
}

void setup_temp_sensor() {
	rcc_periph_clock_enable(RCC_ADC1);
	adc_power_off(ADC1);
	adc_disable_scan_mode(ADC1);
	adc_set_single_conversion_mode(ADC1);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);
	adc_enable_temperature_sensor();
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);
	adc_power_on(ADC1);
	delay(100);
	adc_reset_calibration(ADC1);
	adc_calibrate(ADC1);
}

/* #define RUN_SELFTEST */
/* #define READ_PICC */

int main() {
	/* while (!debugger_attached()) { */
	/* 	__asm("nop"); */
	/* } */

	setup_clocks();
	setup_timers();
	setup_gpio();
	/* setup_spi(); */

	delay(200);
	timer_enable_counter(TIM1);
	timer_enable_counter(TIM2);
	timer_enable_counter(TIM3);

#ifdef RUN_SELFTEST
	MFRC522_Init();
	MFRC522_SelfTest();
	MFRC522_Reset();
#endif

#ifdef READ_PICC
	MFRC522_Init();

	MFRC522_UID_t uid = {0};
	/* uint8_t buffer[64]; */
	/* uint8_t bufLen; */

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
		/* MFRC522_Status status = MIFARE_Read(0x00, buffer, &bufLen); */
		while (!PICC_IsNewCardPresent()) {
		}
		printf("Some card detected! Trying to read...\n");
		MFRC522_Status status = MFRC522_Select(&uid);
		printf("Status: %d\n", status);
		if (status != STATUS_OK) {
			continue;
		}
		printf("Card detected\n");

		/* 		printf("UID size: %u\n", bufLen); */
		/* 		for (uint8_t i = 0; i < bufLen; i++) { */
		/* 			printf("%02x ", buffer[i]); */
		/* 		} */

		printf("UID size: %u\n", uid.size);
		for (uint8_t i = 0; i < uid.size; i++) {
			printf("%02x ", uid.uid[i]);
		}

		printf("\n");
	}

#else

	printf("AHB frequency = %d Hz\n", rcc_ahb_frequency);
	printf("APB1 frequency = %d Hz\n", rcc_apb1_frequency);
	printf("APB2 frequency = %d Hz\n", rcc_apb2_frequency);

	while (1) {
		__asm("nop");
	}

#endif

	return 0;
}
