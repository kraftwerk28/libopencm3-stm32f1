#pragma once

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <stdint.h>

#include "utils.h"

typedef enum {
	RFID_STATUS_OK = 0,
	RFID_STATUS_NO_TAG_ERROR,
	RFID_STATUS_ERROR,
} RC522_Status;

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
#define ComIrqReginterrupt 0x04
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
