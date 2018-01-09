#pragma once
/**
 * @file
 * @brief Analog input/output
 *
 * AIO is the anlog input & output interface to libxpt. It is used to read or
 * set the voltage applied to an AIO pin.
 *
 * @snippet analogin_a0.c Interesting
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "common.h"
#include "gpio.h"

/**
 * Opaque pointer definition to the internal struct _aio. This context refers
 * to one single AIO pin on the board.
 */
typedef struct _aio* xpt_aio_context;

/**
 * Initialise an Analog input device, connected to the specified pin. Aio pins
 * are always 0 indexed reguardless of their position. Check your board mapping
 * for details. An arduino style layout will have A0 as pin14 but AIO0.
 *
 * @param pin Channel number to read ADC inputs
 * @returns aio context or NULL
 */
xpt_aio_context xpt_aio_init(unsigned int pin);

/**
 * Read the input voltage. By default xpt will shift the raw value up or down
 * to a 10 bit value.
 *
 * @param dev The AIO context
 * @returns The current input voltage or -1 for error
 */
int xpt_aio_read(xpt_aio_context dev);

/**
 * Read the input voltage and return it as a normalized float (0.0f-1.0f).
 *
 * @param dev The AIO context
 * @returns The current input voltage as a normalized float (0.0f-1.0f), error
 * will be signaled by -1.0f
 */
float xpt_aio_read_float(xpt_aio_context dev);

/**
 * Close the analog input context, this will free the memory for the context
 *
 * @param dev The AIO context
 * @return Result of operation
 */
xpt_result_t xpt_aio_close(xpt_aio_context dev);

/**
 * Set the bit value which xpt will shift the raw reading
 * from the ADC to. I.e. 10bits
 * @param dev the analog input context
 * @param bits the bits the return from read should be i.e 10
 *
 * @return xpt result type
 */
xpt_result_t xpt_aio_set_bit(xpt_aio_context dev, int bits);

/**
 * Gets the bit value xpt is shifting the analog read to.
 * @param dev the analog input context
 *
 * @return bit value xpt is set return from the read function
 */
int xpt_aio_get_bit(xpt_aio_context dev);

#ifdef __cplusplus
}
#endif
