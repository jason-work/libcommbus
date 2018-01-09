#pragma once

/**
 * @file
 * @brief Inter-Integrated Circuit
 *
 * An i2c context represents a master on an i2c bus and that context can
 * communicate to multiple i2c slaves by configuring the address.
 * @htmlinclude i2c.txt
 *
 * @snippet i2c_HMC5883L.c Interesting
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>

#include "common.h"
#include "gpio.h"

/**
 * Opaque pointer definition to the internal struct _i2c
 */
typedef struct _i2c* xpt_i2c_context;

/**
 * Initialise i2c context, using board defintions
 *
 * @param bus i2c bus to use
 * @return i2c context or NULL
 */
xpt_i2c_context xpt_i2c_init(int bus);

/**
 * Initialise i2c context, passing in the i2c bus to use.
 *
 * @param bus The i2c bus to use i.e. /dev/i2c-2 would be "2"
 * @return i2c context or NULL
 */
xpt_i2c_context xpt_i2c_init_raw(unsigned int bus);

/**
 * Sets the frequency of the i2c context. Most platforms do not support this.
 *
 * @param dev The i2c context
 * @param mode The bus mode
 * @return Result of operation
 */
xpt_result_t xpt_i2c_frequency(xpt_i2c_context dev, xpt_i2c_mode_t mode);

/**
 * Simple bulk read from an i2c context
 *
 * @param dev The i2c context
 * @param data pointer to the byte array to read data in to
 * @param length max number of bytes to read
 * @return length of the read in bytes or -1
 */
int xpt_i2c_read(xpt_i2c_context dev, uint8_t* data, int length);

/**
 * Simple read for a single byte from the i2c context
 *
 * @param dev The i2c context
 * @return The result of the read or -1 if failed
 */
int xpt_i2c_read_byte(xpt_i2c_context dev);

/**
 * Read a single byte from i2c context, from designated register
 *
 * @param dev The i2c context
 * @param command The register
 * @return The result of the read or -1 if failed
 */
int xpt_i2c_read_byte_data(xpt_i2c_context dev, const uint8_t command);

/**
 * Read a single word from i2c context, from designated register
 *
 * @param dev The i2c context
 * @param command The register
 * @return The result of the read or -1 if failed
 */
int xpt_i2c_read_word_data(xpt_i2c_context dev, const uint8_t command);

/**
 * Bulk read from i2c context, starting from designated register
 *
 * @param dev The i2c context
 * @param command The register
 * @param data pointer to the byte array to read data in to
 * @param length max number of bytes to read
 * @return The length in bytes passed to the function or -1
 */
int xpt_i2c_read_bytes_data(xpt_i2c_context dev, uint8_t command, uint8_t* data, int length);

/**
 * Write length bytes to the bus, the first byte in the array is the
 * command/register to write
 *
 * @param dev The i2c context
 * @param data pointer to the byte array to be written
 * @param length the number of bytes to transmit
 * @return Result of operation
 */
xpt_result_t xpt_i2c_write(xpt_i2c_context dev, const uint8_t* data, int length);

/**
 * Write a single byte to an i2c context
 *
 * @param dev The i2c context
 * @param data The byte to write
 * @return Result of operation
 */
xpt_result_t xpt_i2c_write_byte(xpt_i2c_context dev, const uint8_t data);

/**
 * Write a single byte to an i2c context
 *
 * @param dev The i2c context
 * @param data The byte to write
 * @param command The register
 * @return Result of operation
 */
xpt_result_t xpt_i2c_write_byte_data(xpt_i2c_context dev, const uint8_t data, const uint8_t command);

/**
 * Write a single word to an i2c context
 *
 * @param dev The i2c context
 * @param data The word to write
 * @param command The register
 * @return Result of operation
 */
xpt_result_t xpt_i2c_write_word_data(xpt_i2c_context dev, const uint16_t data, const uint8_t command);

/**
 * Sets the i2c slave address.
 *
 * @param dev The i2c context
 * @param address The address to set for the slave (7-bit address)
 * @return Result of operation
 */
xpt_result_t xpt_i2c_address(xpt_i2c_context dev, uint8_t address);

/**
 * De-inits an xpt_i2c_context device
 *
 * @param dev The i2c context
 * @return Result of operation
 */
xpt_result_t xpt_i2c_stop(xpt_i2c_context dev);

#ifdef __cplusplus
}
#endif
