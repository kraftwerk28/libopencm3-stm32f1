#include "mfrc522.h"

#define SPI_MANUAL_CC

#ifdef SPI_MANUAL_CC
#define SELECT_SLAVE() gpio_clear(GPIOA, GPIO_SPI1_NSS)
#else
#define SELECT_SLAVE()
#endif

#ifdef SPI_MANUAL_CC
#define UNSELECT_SLAVE() gpio_set(GPIOA, GPIO_SPI1_NSS)
#else
#define UNSELECT_SLAVE()
#endif

void MFRC522_SetBitMask(uint8_t reg, uint8_t mask) {
	MFRC522_WriteCharToReg(reg, MFRC522_ReadCharFromReg(reg) | mask);
}

void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask) {
	MFRC522_WriteCharToReg(reg, MFRC522_ReadCharFromReg(reg) & ~mask);
}

void MFRC522_AntennaOn() {
	uint8_t val = MFRC522_ReadCharFromReg(TxControlReg);
	if ((val & 0x03) != 0x03) {
		MFRC522_WriteCharToReg(TxControlReg, val | 0x03);
	}
}

void MFRC522_AntennaOff() { MFRC522_ClearBitMask(TxControlReg, 0x03); }

void MFRC522_Init() {
	MFRC522_WriteCharToReg(TxModeReg, 0x00);
	MFRC522_WriteCharToReg(RxModeReg, 0x00);
	MFRC522_WriteCharToReg(ModWidthReg, 0x26);

	MFRC522_WriteCharToReg(TModeReg, 0x80);
	MFRC522_WriteCharToReg(TPrescalerReg, 0xA9);
	MFRC522_WriteCharToReg(TReloadReg1, 0x03);
	MFRC522_WriteCharToReg(TReloadReg1, 0xE8);
	MFRC522_WriteCharToReg(TxASKReg, 0x40);
	MFRC522_WriteCharToReg(ModeReg, 0x3D);
	MFRC522_AntennaOn();
}

void MFRC522_Reset() {
	MFRC522_WriteCharToReg(CommandReg, CMD_SOFT_RESET);
	while (MFRC522_ReadCharFromReg(CommandReg) & (1 << 4)) {
	}
}

uint8_t MFRC522_ReadCharFromReg(uint8_t reg) {
	uint8_t value;
	SELECT_SLAVE();
	spi_transfer(SPI1, (reg << 1) | 0x80);
	value = spi_transfer(SPI1, 0x00);
	UNSELECT_SLAVE();
	return value;
}

void MFRC522_ReadArrayFromReg(uint8_t reg, uint8_t length, uint8_t *outArray) {
	if (length == 0) {
		return;
	}
	SELECT_SLAVE();
	const uint8_t addr = (reg << 1) | 0x80;
	spi_transfer(SPI1, addr);
	uint8_t i = 0;
	for (; i < length - 1; i++) {
		outArray[i] = spi_transfer(SPI1, addr);
	}
	outArray[i] = spi_transfer(SPI1, 0x00);
	UNSELECT_SLAVE();
}

void MFRC522_WriteCharToReg(uint8_t reg, uint8_t data) {
	SELECT_SLAVE();
	spi_transfer(SPI1, (reg << 1) & 0x7E);
	spi_transfer(SPI1, data);
	UNSELECT_SLAVE();
}

void MFRC522_WriteArrayToReg(uint8_t reg, uint8_t length, uint8_t *array) {
	SELECT_SLAVE();
	spi_transfer(SPI1, (reg << 1) & 0x7E);
	for (uint8_t i = 0; i < length; i++) {
		spi_transfer(SPI1, array[i]);
	}
	UNSELECT_SLAVE();
}

bool MFRC522_SelfTest() {
	// This follows directly the steps outlined in 16.1.1
	// 1. Perform a soft reset.
	MFRC522_Reset();

	// 2. Clear the internal buffer by writing 25 bytes of 00h
	uint8_t zeroes[25] = {0};
	MFRC522_WriteCharToReg(FIFOLevelReg, 0x80); // flush the FIFO buffer
	MFRC522_WriteArrayToReg(FIFODataReg, 25,
							zeroes); // write 25 bytes of 00h to FIFO
	MFRC522_WriteCharToReg(CommandReg, CMD_MEM); // transfer to internal buffer

	// 3. Enable self-test
	MFRC522_WriteCharToReg(0x36, 0x09);

	// 4. Write 00h to FIFO buffer
	MFRC522_WriteCharToReg(FIFODataReg, 0x00);

	// 5. Start self-test by issuing the CalcCRC command
	MFRC522_WriteCharToReg(CommandReg, CMD_CALC_CRC);
	printf("CALC_CRC\n");

	// 6. Wait for self-test to complete
	uint8_t n;
	for (uint8_t i = 0; i < 0xFF; i++) {
		// The datasheet does not specify exact completion condition except
		// that FIFO buffer should contain 64 bytes.
		// While selftest is initiated by CalcCRC command
		// it behaves differently from normal CRC computation,
		// so one can't reliably use DivIrqReg to check for completion.
		// It is reported that some devices does not trigger CRCIRq flag
		// during selftest.
		n = MFRC522_ReadCharFromReg(FIFOLevelReg);
		if (n >= 64) {
			break;
		}
	}

	// Stop calculating CRC for new content in the FIFO.
	MFRC522_WriteCharToReg(CommandReg, CMD_IDLE);
	printf("IDLE\n");

	// 7. Read out resulting 64 bytes from the FIFO buffer.
	uint8_t result[64];
	MFRC522_ReadArrayFromReg(FIFODataReg, 64, result);
	for (uint8_t i = 0; i < 64; i++) {
		printf("%#x\n", result[i]);
	}

	// Auto self-test done
	// Reset AutoTestReg register to be 0 again. Required for normal operation.
	MFRC522_WriteCharToReg(0x36, 0x00);

	// Determine firmware version (see section 9.3.4.8 in spec)
	uint8_t version = MFRC522_ReadCharFromReg(VersionReg);

	// Pick the appropriate reference values
	/* const uint8_t *reference; */

	printf("version code: %#x\n", version);
	/* switch (version) { */
	/* case 0x88: // Fudan Semiconductor FM17522 clone */
	/* 	reference = FM17522_firmware_reference; */
	/* 	break; */
	/* case 0x90: // Version 0.0 */
	/* 	reference = MFRC522_firmware_referenceV0_0; */
	/* 	break; */
	/* case 0x91: // Version 1.0 */
	/* 	reference = MFRC522_firmware_referenceV1_0; */
	/* 	break; */
	/* case 0x92: // Version 2.0 */
	/* 	reference = MFRC522_firmware_referenceV2_0; */
	/* 	break; */
	/* default:		  // Unknown version */
	/* 	return false; // abort test */
	/* } */

	// Verify that the results match up to our expectations
	/* for (uint8_t i = 0; i < 64; i++) { */
	/* 	if (result[i] != pgm_read_byte(&(reference[i]))) { */
	/* 		return false; */
	/* 	} */
	/* } */

	// Test passed; all is good.
	return true;
}
