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

const uint8_t SELF_TEST_OUTPUT[] = {
	0x00, 0xEB,	 0x66, 0xBA, 0x57, 0xBF, 0x23, 0x95,  0xD0, 0xE3, 0x0D,
	0x3D, 0x27,	 0x89, 0x5C, 0xDE, 0x9D, 0x3B, 0xA7,  0x00, 0x21, 0x5B,
	0x89, 0x82,	 0x51, 0x3A, 0xEB, 0x02, 0x0C, 0xA5,  0x00, 0x49, 0x7C,
	0x84, 0x4D,	 0xB3, 0xCC, 0xD2, 0x1B, 0x81, 0x5D,  0x48, 0x76, 0xD5,
	0x71, 0x061, 0x21, 0xA9, 0x86, 0x96, 0x83, 0x38,  0xCF, 0x9D, 0x5B,
	0x6D, 0xDC,	 0x15, 0xBA, 0x3E, 0x7D, 0x95, 0x03B, 0x2F,
};

bool MFRC522_SelfTest() {
	// This follows directly the steps outlined in 16.1.1
	// 1. Perform a soft reset.
	MFRC522_Reset();

	// 2. Clear the internal buffer by writing 25 bytes of 00h
	uint8_t zeroes[25] = {0};
	MFRC522_WriteCharToReg(FIFOLevelReg, 0x80); // flush the FIFO buffer
	// write 25 bytes of 00h to FIFO
	MFRC522_WriteArrayToReg(FIFODataReg, 25, zeroes);
	MFRC522_WriteCharToReg(CommandReg, CMD_MEM); // transfer to internal buffer

	// 3. Enable self-test
	MFRC522_WriteCharToReg(0x36, 0x09);

	// 4. Write 00h to FIFO buffer
	MFRC522_WriteCharToReg(FIFODataReg, 0x00);

	// 5. Start self-test by issuing the CalcCRC command
	MFRC522_WriteCharToReg(CommandReg, CMD_CALC_CRC);

	// 6. Wait for self-test to complete
	MFRC522_WaitForFifoLefel(64);

	// Stop calculating CRC for new content in the FIFO.
	MFRC522_WriteCharToReg(CommandReg, CMD_IDLE);

	// 7. Read out resulting 64 bytes from the FIFO buffer.
	uint8_t result[64];
	MFRC522_ReadArrayFromReg(FIFODataReg, 64, result);
	uint8_t version = MFRC522_ReadCharFromReg(VersionReg);
	printf("version code: 0x%02x\n", version);
	for (uint8_t i = 0; i < LEN(result); i++) {
		if (i % 8 == 0 && i > 0) {
			printf("\n");
		}
		printf("0x%02X ", result[i]);
	}
	for (uint8_t i = 0; i < LEN(result); i++) {
		if (SELF_TEST_OUTPUT[i] != result[i]) {
			printf("Test failed (0x%02X != 0x%02X)\n", SELF_TEST_OUTPUT[i],
				   result[i]);
			return false;
		}
	}
	printf("\nSelf-test passed!\n");

	MFRC522_WriteCharToReg(0x36, 0x00);
	return true;
}

void MFRC522_RandomId(uint8_t *outId) {
	/* uint8_t zeros[64] = {0}; */
	/* MFRC522_WriteArrayToReg(FIFODataReg, LEN(zeros), zeros); */
	MFRC522_WriteCharToReg(FIFOLevelReg, 0x80);
	printf("FIFO size = %u\n", MFRC522_ReadCharFromReg(FIFOLevelReg));
	MFRC522_WriteCharToReg(CommandReg, CMD_GEN_RANDOM_ID);
	MFRC522_WriteCharToReg(CommandReg, CMD_MEM);
	/* MFRC522_WaitForFifoLefel(10); */
	MFRC522_ReadArrayFromReg(FIFODataReg, 10, outId);
}

void MFRC522_WaitForFifoLefel(uint8_t fifoSize) {
	while (1) {
		volatile uint8_t lvl = MFRC522_ReadCharFromReg(FIFOLevelReg) & 0x7f;
		/* printf("FIFO size in wait: %u\n", lvl); */
		if (lvl >= fifoSize) {
			break;
		}
	}
}

MFRC522_Status PCD_TransceiveData(uint8_t *sendData, uint8_t sendLen,
								  uint8_t *backData, uint8_t *backLen,
								  uint8_t *validBits, uint8_t rxAlign) {
	uint8_t waitIRq = 0x30;
	return MFRC522_Communicate_PICC(CMD_TRANSCEIVE, waitIRq, sendData, sendLen,
									backData, backLen, validBits, rxAlign,
									false);
}

MFRC522_Status MFRC522_Select(MFRC522_UID_t *uid) {
	bool complete = false, selectDone = false, useCascadeTag = false;
	uint8_t cascadeLevel = 1;
	MFRC522_Status result;
	uint8_t count;
	uint8_t checkBit;
	uint8_t index;
	/* The first index in uid->uidByte[] that is used in the
	 * current Cascade Level. */
	uint8_t uidIndex;
	/* The number of known UID bits in the current
	 * Cascade Level. */
	int8_t currentLevelKnownBits;
	/* The SELECT/ANTICOLLISION commands uses a 7 byte
	 * standard frame + 2 bytes CRC_A */
	uint8_t buffer[9];
	/* The number of bytes used in the buffer, ie the number
	 * of bytes to transfer to the FIFO. */
	uint8_t bufferUsed;
	/* Used in BitFramingReg. Defines the bit position for the
	 * first bit received.  */
	uint8_t rxAlign;
	/* Used in BitFramingReg. The number of valid bits in
	 * the last transmitted byte.  */
	uint8_t txLastBits;
	uint8_t *responseBuffer;
	uint8_t responseLength;

	MFRC522_ClearBitMask(CollReg, 0x80);

	while (!complete) {
		switch (cascadeLevel) {
		case 1: {
			buffer[0] = PICC_CMD_SEL_CL1;
			uidIndex = 0;
			break;
		}
		case 2: {
			buffer[0] = PICC_CMD_SEL_CL2;
			uidIndex = 3;
			break;
		}
		case 3: {
			buffer[0] = PICC_CMD_SEL_CL3;
			uidIndex = 6;
			break;
		}
		default:
			return STATUS_INTERNAL_ERROR;
		}
		currentLevelKnownBits = -(8 * uidIndex);
		if (currentLevelKnownBits) {
			currentLevelKnownBits = 0;
		}
		index = 2;
		if (useCascadeTag) {
			buffer[index++] = PICC_CMD_CT;
		}
		// The number of bytes needed to represent
		// the known bits for this level.
		uint8_t bytesToCopy =
			currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0);

		if (bytesToCopy) {
			// Max 4 bytes in each Cascade Level.
			// Only 3 left if we use the Cascade Tag
			uint8_t maxBytes = useCascadeTag ? 3 : 4;

			if (bytesToCopy > maxBytes) {
				bytesToCopy = maxBytes;
			}
			for (count = 0; count < bytesToCopy; count++) {
				buffer[index++] = uid->uid[uidIndex + count];
			}
		}
		// Now that the data has been copied we need to include the 8 bits in CT
		// in currentLevelKnownBits
		if (useCascadeTag) {
			currentLevelKnownBits += 8;
		}

		// Repeat anti collision loop until we can transmit all UID bits + BCC
		// and receive a SAK - max 32 iterations.
		selectDone = false;
		while (!selectDone) {
			// Find out how many bits and bytes to send and receive.
			// All UID bits in this Cascade Level are known. This is a
			// SELECT.
			if (currentLevelKnownBits >= 32) {
				// Serial.print(F("SELECT: currentLevelKnownBits="));
				// Serial.println(currentLevelKnownBits, DEC);
				// NVB - Number of Valid Bits: Seven whole bytes
				buffer[1] = 0x70;
				// Calculate BCC - Block Check Character
				buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
				// Calculate CRC_A
				result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
				if (result != STATUS_OK) {
					return result;
				}
				// 0 => All 8 bits are valid.
				txLastBits = 0;
				bufferUsed = 9;
				// Store response in the last 3 bytes of buffer (BCC and CRC_A -
				// not needed after tx)
				responseBuffer = &buffer[6];
				responseLength = 3;
			} else {
				// ^ This is an ANTICOLLISION.
				// Serial.print(F("ANTICOLLISION: currentLevelKnownBits="));
				// Serial.println(currentLevelKnownBits, DEC);
				txLastBits = currentLevelKnownBits % 8;
				// Number of whole bytes in the UID part.
				count = currentLevelKnownBits / 8;
				// Number of whole bytes: SEL + NVB + UIDs
				// NVB - Number of Valid Bits
				index = 2 + count;
				buffer[1] = (index << 4) + txLastBits;
				bufferUsed = index + (txLastBits ? 1 : 0);
				// Store response in the unused part of buffer
				responseBuffer = &buffer[index];
				responseLength = sizeof(buffer) - index;
			}

			// Set bit adjustments
			// Having a separate variable is overkill. But
			// it makes the next line easier to read.
			rxAlign = txLastBits;

			// RxAlign = BitFramingReg[6..4]. TxLastBits =
			// BitFramingReg[2..0]
			MFRC522_WriteCharToReg(BitFramingReg, (rxAlign << 4) + txLastBits);

			// Transmit the buffer and receive the response.
			result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer,
										&responseLength, &txLastBits, rxAlign);

			// More than one PICC in the field
			// => collision.
			if (result == STATUS_COLLISION) {

				// CollReg[7..0] bits are: ValuesAfterColl
				// reserved CollPosNotValid CollPos[4:0]
				uint8_t valueOfCollReg = MFRC522_ReadCharFromReg(CollReg);

				// CollPosNotValid
				// Without a valid collision
				// position we cannot continue
				if (valueOfCollReg & 0x20) {
					return STATUS_COLLISION;
				}

				// Values 0-31, 0 means bit 32.
				uint8_t collisionPos = valueOfCollReg & 0x1F;
				if (collisionPos == 0) {
					collisionPos = 32;
				}

				// No progress - should not happen
				if (collisionPos <= currentLevelKnownBits) {
					return STATUS_INTERNAL_ERROR;
				}
				// Choose the PICC with the bit set.
				currentLevelKnownBits = collisionPos;
				// The bit to modify
				count = currentLevelKnownBits % 8;
				checkBit = (currentLevelKnownBits - 1) % 8;
				// First byte is index 0.
				index = 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0);
				buffer[index] |= (1 << checkBit);
			} else if (result != STATUS_OK) {
				return result;
			} else {
				// STATUS_OK
				// This was a SELECT.
				// No more anticollision
				if (currentLevelKnownBits >= 32) {
					selectDone = true;
					// We continue below outside the while.

					// This was an ANTICOLLISION.
				} else {
					// We now have all 32 bits of the UID in this Cascade Level
					currentLevelKnownBits = 32;
					// Run loop again to do the SELECT.
				}
			}
		} // End of while (!selectDone)

		// We do not check the CBB - it was constructed by us above.

		// Copy the found UID bytes from buffer[] to uid->uidByte[]
		index = (buffer[2] == PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
		bytesToCopy = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
		for (count = 0; count < bytesToCopy; count++) {
			uid->uid[uidIndex + count] = buffer[index++];
		}

		// Check response SAK (Select Acknowledge)
		if (responseLength != 3 ||
			txLastBits != 0) { // SAK must be exactly 24 bits (1 byte + CRC_A).
			return STATUS_ERROR;
		}
		// Verify CRC_A - do our own calculation and store the control in
		// buffer[2..3] - those bytes are not needed anymore.
		result = PCD_CalculateCRC(responseBuffer, 1, &buffer[2]);
		if (result != STATUS_OK) {
			return result;
		}
		if ((buffer[2] != responseBuffer[1]) ||
			(buffer[3] != responseBuffer[2])) {
			return STATUS_CRC_WRONG;
		}
		if (responseBuffer[0] &
			0x04) { // Cascade bit set - UID not complete yes
			cascadeLevel++;
		} else {
			complete = true;
			uid->sak = responseBuffer[0];
		}
	} // End of while (!uidComplete)

	// Set correct uid->size
	uid->size = 3 * cascadeLevel + 1;

	return STATUS_OK;
}

MFRC522_Status PCD_CalculateCRC(uint8_t *data, uint8_t length,
								uint8_t *result) {
	MFRC522_WriteCharToReg(CommandReg, CMD_IDLE);
	// Clear the CRCIRq interrupt request bit
	MFRC522_WriteCharToReg(DivIrqReg, 0x04);

	// FlushBuffer = 1, FIFO initialization
	MFRC522_WriteCharToReg(FIFOLevelReg, 0x80);
	// Write data to the FIFO
	MFRC522_WriteArrayToReg(FIFODataReg, length, data);
	// Start the calculation
	MFRC522_WriteCharToReg(CommandReg, CMD_CALC_CRC);

	// Wait for the CRC calculation to complete. Each iteration of the
	// while-loop takes 17.73μs.
	// TODO check/modify for other architectures than Arduino Uno 16bit

	// Wait for the CRC calculation to complete. Each iteration of the
	// while-loop takes 17.73us.
	for (uint32_t i = 0xffffff; i > 0; i--) {
		// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq reserved
		// CRCIRq reserved reserved
		uint8_t n = MFRC522_ReadCharFromReg(DivIrqReg);
		// CRCIRq bit set - calculation done
		if (n & 0x04) {
			// Stop calculating CRC for new content in the FIFO.
			MFRC522_WriteCharToReg(CommandReg, CMD_IDLE);
			// Transfer the result from the registers to the result buffer
			result[0] = MFRC522_ReadCharFromReg(CRCResultReg1);
			result[1] = MFRC522_ReadCharFromReg(CRCResultReg2);
			return STATUS_OK;
		}
	}
	printf("CRC timeout\n");
	// 89ms passed and nothing happend. Communication with the MFRC522 might be
	// down.
	return STATUS_TIMEOUT;
}

MFRC522_Status MFRC522_Communicate_PICC(uint8_t command, uint8_t waitIRq,
										uint8_t *sendData, uint8_t sendLen,
										uint8_t *backData, uint8_t *backLen,
										uint8_t *validBits, uint8_t rxAlign,
										bool checkCRC) {

	// Prepare values for BitFramingReg
	uint8_t txLastBits = validBits ? *validBits : 0;
	// RxAlign = BitFramingReg[6..4].
	// TxLastBits = BitFramingReg[2..0]
	uint8_t bitFraming = (rxAlign << 4) + txLastBits;

	// Stop any active command.
	MFRC522_WriteCharToReg(CommandReg, CMD_IDLE);
	// Clear all seven interrupt request bits
	MFRC522_WriteCharToReg(ComIrqReg, 0x7F);
	// FlushBuffer = 1, FIFO initialization
	MFRC522_WriteCharToReg(FIFOLevelReg, 0x80);
	// Write sendData to the FIFO
	MFRC522_WriteArrayToReg(FIFODataReg, sendLen, sendData);
	// Bit adjustments
	MFRC522_WriteCharToReg(BitFramingReg, bitFraming);
	// Execute the command
	MFRC522_WriteCharToReg(CommandReg, command);

	if (command == CMD_TRANSCEIVE) {
		// StartSend=1, transmission of data starts
		MFRC522_SetBitMask(BitFramingReg, 0x80);
	}

	// Wait for the command to complete.
	// In PCD_Init() we set the TAuto flag in TModeReg. This means the timer
	// automatically starts when the PCD stops transmitting. Each iteration of
	// the do-while-loop takes 17.86μs.
	// TODO check/modify for other architectures than Arduino Uno 16bit
	uint32_t i;
	for (i = 0xffffff; i > 0; i--) {
		// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq
		// HiAlertIRq LoAlertIRq ErrIRq TimerIRq
		uint8_t n = MFRC522_ReadCharFromReg(ComIrqReg);

		// One of the interrupts that signal success has been set.
		if (n & waitIRq) {
			break;
		}
		// Timer interrupt - nothing received in 25ms
		if (n & 0x01) {
			return STATUS_TIMEOUT;
		}
	}
	// 35.7ms and nothing happend. Communication with the MFRC522 might be down.
	if (i == 0) {
		return STATUS_TIMEOUT;
	}

	// Stop now if any errors except collisions were detected.
	// ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl
	// CollErr CRCErr ParityErr ProtocolErr
	uint8_t errorRegValue = MFRC522_ReadCharFromReg(ErrorReg);

	// BufferOvfl ParityErr ProtocolErr
	if (errorRegValue & 0x13) {
		return STATUS_ERROR;
	}

	uint8_t _validBits = 0;

	// If the caller wants data back, get it from the MFRC522.
	if (backData && backLen) {
		// Number of bytes in the FIFO
		uint8_t n = MFRC522_ReadCharFromReg(FIFOLevelReg);
		if (n > *backLen) {
			return STATUS_NO_ROOM;
		}
		// Number of bytes returned
		*backLen = n;
		// Get received data from FIFO
		MFRC522_ReadArrayFromReg(FIFODataReg, n, backData);
		// RxLastBits[2:0] indicates the number of valid bits
		// in the last received byte. If this value is 000b,
		// the whole byte is valid.
		_validBits = MFRC522_ReadCharFromReg(ControlReg) & 0x07;

		if (validBits) {
			*validBits = _validBits;
		}
	}

	// Tell about collisions
	// CollErr
	if (errorRegValue & 0x08) {
		return STATUS_COLLISION;
	}
	// Perform CRC_A validation if requested.
	if (backData && backLen && checkCRC) {
		// In this case a MIFARE Classic NAK is not OK.
		if (*backLen == 1 && _validBits == 4) {
			return STATUS_MIFARE_NACK;
		}
		// We need at least the CRC_A value and all 8 bits of the last byte must
		// be received.
		if (*backLen < 2 || _validBits != 0) {
			return STATUS_CRC_WRONG;
		}
		// Verify CRC_A - do our own calculation and store the control in
		// controlBuffer.
		uint8_t controlBuffer[2] = {0};
		MFRC522_Status status =
			PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
		if (status != STATUS_OK) {
			return status;
		}
		if ((backData[*backLen - 2] != controlBuffer[0]) ||
			(backData[*backLen - 1] != controlBuffer[1])) {
			return STATUS_CRC_WRONG;
		}
	}

	return STATUS_OK;
}

MFRC522_Status MIFARE_Read(uint8_t blockAddr, uint8_t *buffer,
						   uint8_t *bufferSize) {
	MFRC522_Status result;

	// Sanity check
	if (buffer == 0 || *bufferSize < 18) {
		return STATUS_NO_ROOM;
	}

	// Build command buffer
	buffer[0] = PICC_CMD_MF_READ;
	buffer[1] = blockAddr;
	// Calculate CRC_A
	result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
	if (result != STATUS_OK) {
		return result;
	}

	// Transmit the buffer and receive the response, validate CRC_A.
	return PCD_TransceiveData(buffer, 4, buffer, bufferSize, 0, 0);
}

MFRC522_Status PICC_RequestA(uint8_t *bufferATQA, uint8_t *bufferSize) {
	uint8_t command = PICC_CMD_REQA;
	uint8_t validBits;
	MFRC522_Status status;

	// The ATQA response is 2 bytes long.
	if (bufferATQA == 0 || *bufferSize < 2) {
		return STATUS_NO_ROOM;
	}
	// ValuesAfterColl=1 => Bits received after collision are cleared.
	MFRC522_ClearBitMask(CollReg, 0x80);
	// For REQA and WUPA we need the short frame format - transmit only 7 bits
	// of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
	validBits = 7;
	status =
		PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits, 0);
	if (status != STATUS_OK) {
		return status;
	}
	// ATQA must be exactly 16 bits.
	if (*bufferSize != 2 || validBits != 0) {
		return STATUS_ERROR;
	}
	return STATUS_OK;
}

bool PICC_IsNewCardPresent() {
	uint8_t bufferATQA[2];
	uint8_t bufferSize = sizeof(bufferATQA);

	// Reset baud rates
	MFRC522_WriteCharToReg(TxModeReg, 0x00);
	MFRC522_WriteCharToReg(RxModeReg, 0x00);
	// Reset ModWidthReg
	MFRC522_WriteCharToReg(ModWidthReg, 0x26);

	MFRC522_Status result = PICC_RequestA(bufferATQA, &bufferSize);
	return (result == STATUS_OK || result == STATUS_COLLISION);
}
