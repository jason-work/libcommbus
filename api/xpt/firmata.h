#pragma once

/**
 * @file
 * @brief Firmata IO
 *
 * Firmata IO lets you SYSEX messages construct and ask for a callback on a
 * SYSEX messages. This is meant to provide a way to call custom firmata APIs
 * especially using the Custom firmata API
 *
 * @snippet firmata_curie_imu.c Interesting
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/**
 * Opaque pointer definition to the internal struct _firmata. This context
 * refers to one firmata 'extension' letting you write/return SYSEX messages
 * directly
 */
typedef struct _firmata* xpt_firmata_context;

/**
 * Initialise firmata context on a feature. This feature is what will be
 * listened on if you request a response callback
 *
 * @param feature firmata feature
 * @return firmata context or NULL
 */
xpt_firmata_context xpt_firmata_init(int feature);

/**
 * Sends a custom SYSEX message to the firmata board.
 *
 * @param dev The Firmata context
 * @param msg The SYSEX message
 * @param length The length of the sysex message
 */
xpt_result_t xpt_firmata_write_sysex(xpt_firmata_context dev, char* msg, int length);

/**
 * Set a callback on 'feature'. This function is not thread safe and threads
 * calling it need to make sure they are the only thread calling this.
 *
 * @param dev The Firmata context
 * @param fptr Function pointer to function to be called when interrupt is
 * triggered, the returned buffer and length are the arguments.
 * @return Result of operation
 */
xpt_result_t xpt_firmata_response(xpt_firmata_context dev, void (*fptr)(uint8_t*, int));

/**
 * Stop getting events on feature. This is more efficient than xpt_firmata_close
 * as it can be re-enabled without adding a feature
 *
 * @param dev The Firmata context
 * @return Result of operation
 */
xpt_result_t xpt_firmata_response_stop(xpt_firmata_context dev);

/**
 * Free all firmata handle resources, this will leave an element in an array
 * internally that will be skipped, avoid closing many firmata contexts often
 * as there is a cost to doing this
 *
 * @param dev The Firmata context
 * @return Result of operation
 */
xpt_result_t xpt_firmata_close(xpt_firmata_context dev);

#ifdef __cplusplus
}
#endif
