#pragma once

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <stdint.h>

#include "utils.h"

typedef enum {
	STATUS_OK,			   // Success
	STATUS_ERROR,		   // Error in communication
	STATUS_COLLISION,	   // Collission detected
	STATUS_TIMEOUT,		   // Timeout in communication.
	STATUS_NO_ROOM,		   // A buffer is not big enough.
	STATUS_INTERNAL_ERROR, // Internal error in the code. Should not happen ;-)
	STATUS_INVALID,		   // Invalid argument.
	STATUS_CRC_WRONG,	   // The CRC_A does not match
	STATUS_MIFARE_NACK = 0xff // A MIFARE PICC responded with NAK.
} MFRC522_Status;

typedef struct {
	uint8_t size;
	uint8_t uid[10];
	uint8_t sak;
} MFRC522_UID_t;

/* Commands: */

/* no action, cancels current command execution */
#define CMD_IDLE 0b0000

/* stores 25 bytes into the internal buffer */
#define CMD_MEM 0b0001

/* generates a 10-byte random ID number */
#define CMD_GEN_RANDOM_ID 0b0010

/* activates the CRC coprocessor or performs a self test */
#define CMD_CALC_CRC 0b0011

/* transmits data from the FIFO buffer */
#define CMD_TRANSMIT 0b0100

/* no command change, can be used to modify the
 * CommandReg register bits without affecting the command,
 * for example, the PowerDown bit */
#define CMD_NOCMDCHANGE 0b0111

/* activates the receiver circuits */
#define CMD_RECEIVE 0b1000

/* transmits data from FIFO buffer to antenna and automatically
 * activates the receiver after transmission */
#define CMD_TRANSCEIVE 0b1100

/* performs the MIFARE standard authentication as a reader */
#define CMD_MFAUTHENT 0b1110

/* resets the MFRC522 */
#define CMD_SOFT_RESET 0b1111

/* Registers: */

/* Command and status registers: */
/* reserved for future useTable 21 on page 38 */
#define Reserved1 0x00
/* starts and stops command executionTable 23 on page 38 */
#define CommandReg 0x01
/* enable and disable interrupt request control bitsTable 25 on page 38 */
#define ComlEnReg 0x02
/* enable and disable interrupt request control bitsTable 27 on page 39 */
#define DivlEnReg 0x03
/* request bitsTable 29 on page 39 */
#define ComIrqReg 0x04
/* interrupt request bitsTable 31 on page 40 */
#define DivIrqReg 0x05
/* error bits showing the error status
 * of the last command executedTable 33 on page 41 */
#define ErrorReg 0x06
/* communication status bitsTable 35 on page 42 */
#define Status1Reg 0x07
/* receiver and transmitter status bitsTable 37 on page 43 */
#define Status2Reg 0x08
/* input and output of 64 byte FIFO bufferTable 39 on page 44 */
#define FIFODataReg 0x09
/* number of bytes stored in the FIFO bufferTable 41 on page 44 */
#define FIFOLevelReg 0x0A
/* level for FIFO underflow and overflow warningTable 43 on page 44 */
#define WaterLevelReg 0x0B
/* miscellaneous control registersTable 45 on page 45 */
#define ControlReg 0x0C
/* adjustments for bit-oriented framesTable 47 on page 46 */
#define BitFramingReg 0x0D
/* bit position of the first bit-collision detected
 * on the RF interfaceTable 49 on page 46 */
#define CollReg 0x0E
/* reserved for future useTable 51 on page 47 */
#define Reserved2 0x0F

/* Command registers: */
/* defines general modes for transmitting and receivingTable 55 on page 48 */
#define Reserved 0x10
/* defines general modes for transmitting and receiving */
#define ModeReg 0x11
/* defines transmission data rate and framingTable 57 on page 48 */
#define TxModeReg 0x12
/* defines reception data rate and framingTable 59 on page 49 */
#define RxModeReg 0x13
/* controls the logical behavior of the antenna driver pins TX1
 * and TX2 */
#define TxControlReg 0x14

/* controls the setting of the transmission modulation */
#define TxASKReg 0x15
/* selects the internal sources for the antenna driver */
#define TxSelReg 0x16
/* selects internal receiver settingsTable 67 on page 52 */
#define RxSelReg 0x17
/* selects thresholds for the bit decoderTable 69 on page 53 */
#define RxThresholdReg 0x18
/* defines demodulator settingsTable 71 on page 53 */
#define DemodReg 0x19
/* reserved for future useTable 73 on page 54 */
#define Reserved3 0x1A
/* reserved for future useTable 75 on page 54 */
#define Reserved4 0x1B
/* controls some MIFARE communication transmit parameters Table 77 on page 55 */
#define MfTxReg 0x1C
/* controls some MIFARE communication receive parametersTable 79 on page 55 */
#define MfRxReg 0x1D
/* reserved for future useTable 81 on page 55 */
#define Reserved5 0x1E
/* selects the speed of the serial UART interfaceTable 83 on page 55 */
#define SerialSpeedReg 0x1F

/* Configuration registers: */
/* reserved for future use */
#define Reserved6 0x20
/* shows the MSB and LSB values of the CRC calculationTable 87 on page 57
 * Table 89 on page 57 */
#define CRCResultReg1 0x21
#define CRCResultReg2 0x22
/* reserved for future useTable 91 on page 58 */
#define Reserved7 0x23
/* controls the ModWidth settingTable 93 on page 58 */
#define ModWidthReg 0x24
/* reserved for future useTable 95 on page 58 */
#define Reserved8 0x25
/* configures the receiver gainTable 97 on page 59 */
#define RFCfgReg 0x26
/* selects the conductance of the antenna driver pins TX1 and TX2
 * for modulationTable 99 on page 59 */
#define GsNReg 0x27
/* defines the conductance of the p-driver output during
 * periods of no modulationTable 101 on page 60 */
#define CWGsPReg 0x28
/* defines the conductance of the p-driver output during
 * periods of modulation
 * */
#define ModGsPReg 0x29
/* defines settings for the internal timer */
#define TModeReg 0x2A
/* defines settings for the internal timer */
#define TPrescalerReg 0x2B
/* defines the 16-bit timer reload value */
#define TReloadReg1 0x2C
/* defines the 16-bit timer reload value */
#define TReloadReg2 0x2D
/* shows the 16-bit timer value */
#define TCounterValReg1 0x2E
/* shows the 16-bit timer value */
#define TCounterValReg2 0x2F

/* Test registers */
/* shows the software version */
#define VersionReg 0x37

/* PICC commands: */

// REQuest command, Type A. Invites PICCs in state IDLE to go to READY and
// prepare for anticollision or selection. 7 bit frame.
#define PICC_CMD_REQA 0x26
// Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to
// READY(*) and prepare for anticollision or selection. 7 bit frame.
#define PICC_CMD_WUPA 0x52
// Cascade Tag. Not really a command, but used during anti collision.
#define PICC_CMD_CT 0x88
// Anti collision/Select, Cascade Level 1
#define PICC_CMD_SEL_CL1 0x93
// Anti collision/Select, Cascade Level 2
#define PICC_CMD_SEL_CL2 0x95
// Anti collision/Select, Cascade Level 3
#define PICC_CMD_SEL_CL3 0x97
// HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
#define PICC_CMD_HLTA 0x50
// Request command for Answer To Reset.
// The commands used for MIFARE Classic (from
// http://www.mouser.com/ds/2/302/MF1S503x-89574.pdf, Section 9) Use
// PCD_MFAuthent to authenticate access to a sector, then use these commands to
// read/write/modify the blocks on the sector. The read/write commands can also
// be used for MIFARE Ultralight.
#define PICC_CMD_RATS 0xE0
// Perform authentication with Key A
#define PICC_CMD_MF_AUTH_KEY_A 0x60
// Perform authentication with Key B
#define PICC_CMD_MF_AUTH_KEY_B 0x61
// Reads one 16 byte block from the authenticated sector of the PICC. Also used
// for MIFARE Ultralight.
#define PICC_CMD_MF_READ 0x30
// Writes one 16 byte block to the authenticated sector of the PICC. Called
// "COMPATIBILITY WRITE" for MIFARE Ultralight.
#define PICC_CMD_MF_WRITE 0xA0
// Decrements the contents of a block and stores the result in the internal data
// register.
#define PICC_CMD_MF_DECREMENT 0xC0
// Increments the contents of a block and stores the result in the internal data
// register.
#define PICC_CMD_MF_INCREMENT 0xC1
// Reads the contents of a block into the internal data register.
#define PICC_CMD_MF_RESTORE 0xC2
// Writes the contents of the internal data register to a block.
// The commands used for MIFARE Ultralight (from
// http://www.nxp.com/documents/data_sheet/MF0ICU1.pdf, Section 8.6) The
// PICC_CMD_MF_READ and PICC_CMD_MF_WRITE can also be used for MIFARE
// Ultralight.
#define PICC_CMD_MF_TRANSFER 0xB0
// Writes one 4 byte page to the PICC.
#define PICC_CMD_UL_WRITE 0xA2

void MFRC522_SetBitMask(uint8_t reg, uint8_t mask);
void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask);

uint8_t MFRC522_ReadCharFromReg(uint8_t reg);
void MFRC522_ReadArrayFromReg(uint8_t reg, uint8_t length, uint8_t *outArray);

void MFRC522_WriteCharToReg(uint8_t reg, uint8_t value);
void MFRC522_WriteArrayToReg(uint8_t reg, uint8_t length, uint8_t *array);

void MFRC522_Init();
void MFRC522_Reset();
void MFRC522_AntennaOn();
void MFRC522_AntennaOff();
bool MFRC522_SelfTest();
void MFRC522_RandomId(uint8_t *outId);
void MFRC522_WaitForFifoLefel(uint8_t fifoSize);

MFRC522_Status PCD_TransceiveData(
	// Pointer to the data to transfer to the FIFO.
	uint8_t *sendData,
	// Number of bytes to transfer to the FIFO.
	uint8_t sendLen,
	// nullptr or pointer to buffer if data should be read
	// back after executing the command.
	uint8_t *backData,
	// In: Max number of bytes to write to *backData. Out:
	// The number of bytes returned.
	uint8_t *backLen,
	// In/Out: The number of valid bits in the last byte.
	// 0 for 8 valid bits. Default nullptr.
	uint8_t *validBits,
	// In: Defines the bit position in backData[0] for the
	// first bit received. Default 0.
	uint8_t rxAlign);

MFRC522_Status MFRC522_Select(MFRC522_UID_t *uid);

MFRC522_Status PCD_CalculateCRC(uint8_t *data, uint8_t length, uint8_t *result);

MFRC522_Status MFRC522_Communicate_PICC(
	// The command to execute. One of the PCD_Command enums.
	uint8_t command,
	// The bits in the ComIrqReg register that signals
	// successful completion of the command.
	uint8_t waitIRq,
	// Pointer to the data to transfer to the FIFO.
	uint8_t *sendData,
	// Number of bytes to transfer to the FIFO.
	uint8_t sendLen,
	// nullptr or pointer to buffer if data should be read
	// back after executing the command.
	uint8_t *backData,
	// In: Max number of bytes to write to *backData. Out:
	// The number of bytes returned.
	uint8_t *backLen,
	// In/Out: The number of valid bits in the last byte.
	// 0 for 8 valid bits.
	uint8_t *validBits,
	// In: Defines the bit position in backData[0] for the
	// first bit received. Default 0.
	uint8_t rxAlign,
	// In: True => The last two bytes of the response is assumed
	// to be a CRC_A that must be validated.
	bool checkCRC);

MFRC522_Status MIFARE_Read(
	// MIFARE Classic: The block (0-0xff) number. MIFARE Ultralight: The first
	// page to return data from.
	uint8_t blockAddr,
	// The buffer to store the data in
	uint8_t *buffer,
	// Buffer size, at least 18 bytes. Also number of bytes returned if
	// STATUS_OK.
	uint8_t *bufferSize);

MFRC522_Status PICC_RequestA(
	// The buffer to store the ATQA (Answer to request) in
	uint8_t *bufferATQA,
	// Buffer size, at least two bytes. Also number of bytes returned if
	// STATUS_OK.
	uint8_t *bufferSize);

bool PICC_IsNewCardPresent();
