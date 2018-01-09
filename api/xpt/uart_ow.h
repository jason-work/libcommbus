#pragma once

/**
 * @file
 * @brief UART OW module
 *
 * This module allows one to use XPT's UART support in order to
 * interact with Dallas 1-wire compliant devices on a 1-wire bus.  It
 * makes use of the UART for timing purposes.  The principle of
 * operation is described here:
 * https://www.maximintegrated.com/en/app-notes/index.mvp/id/214
 *
 * It is important the you use a UART with CMOS/TTL level voltages
 * (3.3v/5v) RX and TX lines.  DO NOT use standard RS232 level
 * voltages, or you are going to have a bad day.
 *
 * In addition, a diode should be placed across the RX and
 * TX lines like so:
 *
 * -|
 * U|
 * A| TX---|<--+
 * R|          |
 * T| RX-------o--------o 1-wire data bus
 * -|
 *
 * The diode on TX is a 1N4148 (cheap and common), with the cathode
 * connected to TX, and the anode connected to RX and the rest of the
 * 1-wire data line.
 *
 * @snippet uart_ow.c Interesting
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "common.h"
#include "uart.h"

/** 8 bytes (64 bits) for a device rom code */
#define XPT_UART_OW_ROMCODE_SIZE 8

/** for now, we simply use the normal XPT UART context */
typedef struct _xpt_uart_ow {
    /** Uart Context */
    xpt_uart_context uart;
    /** search state */
    unsigned char ROM_NO[XPT_UART_OW_ROMCODE_SIZE]; /* 8 byte (64b) rom code */
    /** Context laxt discrepancy  */
    int LastDiscrepancy;
    /** Context las family discrepancy */
    int LastFamilyDiscrepancy;
    /** Context las device flag */
    xpt_boolean_t LastDeviceFlag;
} *xpt_uart_ow_context;

/**
 * UART One Wire ROM related Command bytes
 */
typedef enum {
    XPT_UART_OW_CMD_READ_ROM = 0x33,         /**< read rom, when only one device on bus */
    XPT_UART_OW_CMD_MATCH_ROM = 0x55,        /**< match a specific rom code */
    XPT_UART_OW_CMD_SKIP_ROM = 0xcc,         /**< skip match/search rom */
    XPT_UART_OW_CMD_SEARCH_ROM_ALARM = 0xec, /**< search all roms in alarm state */
    XPT_UART_OW_CMD_SEARCH_ROM = 0xf0        /**< search all rom codes */
} xpt_uart_ow_rom_cmd_t;

/**
 * Initialise uart_ow_context, uses UART board mapping
 *
 * @param uart the index of the uart set to use
 * @return uart_ow context or NULL
 */
xpt_uart_ow_context xpt_uart_ow_init(int uart);

/**
 * Initialise a raw uart_ow_context. No board setup.
 *
 * @param path for example "/dev/ttyS0"
 * @return uart_ow context or NULL
 */
xpt_uart_ow_context xpt_uart_ow_init_raw(const char* path);

/**
 * Get char pointer with tty device path within Linux
 * For example. Could point to "/dev/ttyS0"
 *
 * @param dev uart_ow context
 * @return char pointer of device path
 */
const char* xpt_uart_ow_get_dev_path(xpt_uart_ow_context dev);

/**
 * Destroy a xpt_uart_ow_context
 *
 * @param dev uart_ow context
 * @return xpt_result_t
 */
xpt_result_t xpt_uart_ow_stop(xpt_uart_ow_context dev);

/**
 * Read a byte from the 1-wire bus
 *
 * @param dev uart_ow context
 * @return the byte read or -1 for error
 */
int xpt_uart_ow_read_byte(xpt_uart_ow_context dev);

/**
 * Write a byte to a 1-wire bus
 *
 * @param dev uart_ow context
 * @param byte the byte to write to the bus
 * @return the byte read back during the time slot or -1 for error
 */
int xpt_uart_ow_write_byte(xpt_uart_ow_context dev, uint8_t byte);

/**
 * Write a bit to a 1-wire bus and read a bit corresponding to the
 * time slot back.  This is possible due to the way we wired the TX
 * and RX together with a diode, forming a loopback.
 *
 * @param dev uart_ow context
 * @param bit the bit to write to the bus
 * @return the bit read back during the time slot or -1 for error
 */
int xpt_uart_ow_bit(xpt_uart_ow_context dev, uint8_t bit);

/**
 * Send a reset pulse to the 1-wire bus and test for device presence
 *
 * @param dev uart_ow context
 * @return one of the xpt_result_t values
 */
xpt_result_t xpt_uart_ow_reset(xpt_uart_ow_context dev);

/**
 * Begin a rom code search of the 1-wire bus.  This function
 * implements the 1-wire search algorithm.  See the uart_ow.c example
 * for an idea on how to use this function to identify all devices
 * present on the bus.
 *
 * @param dev uart_ow context
 * @param start true to start a new search from scratch, false to
 * continue an existing search
 * @param id the 8-byte rom code id of the current matched device when
 * a device is found
 * @return one of the xpt_result_t values
 */
xpt_result_t xpt_uart_ow_rom_search(xpt_uart_ow_context dev, xpt_boolean_t start, uint8_t* id);

/**
 * Send a command byte to a device on the 1-wire bus
 *
 * @param dev uart_ow context
 * @param command the command byte to send
 * @param id the rom code id of the device to receive the command,
 * NULL for all devices on the bus
 * @return one of the xpt_result_t values
 */
xpt_result_t xpt_uart_ow_command(xpt_uart_ow_context dev, uint8_t command, uint8_t* id);

/**
 * Perform a Dallas 1-wire compliant CRC8 computation on a buffer
 *
 * @param buffer the buffer containing the data
 * @param length the length of the buffer
 * @return the computed CRC
 */
uint8_t xpt_uart_ow_crc8(uint8_t* buffer, uint16_t length);

#ifdef __cplusplus
}
#endif
