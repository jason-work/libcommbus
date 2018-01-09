#pragma once

/**
 * @file
 * @brief LED module
 *
 * LED is the Light Emitting Diode interface to libxpt. It is used to
 * access the on board LED's via sysfs.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include <stdio.h>

/**
 * Opaque pointer definition to the internal struct _led
 */
typedef struct _led* xpt_led_context;

/**
 * Initialise led_context, based on led function name.
 * The structure of LED entry in sysfs is "devicename:colour:function"
 * This api expects only one unique LED identifier which would be
 * "function" name most often. For instance, `xpt_led_init("user4");`
 *
 *  @param led Name of the LED
 *  @returns LED context or NULL
 */
xpt_led_context xpt_led_init(const char* led);

/**
 * Set LED brightness
 *
 *  @param dev LED context
 *  @param value Integer value to write
 *  @returns Result of operation
 */
xpt_result_t xpt_led_set_brightness(xpt_led_context dev, int value);

/**
 * Read LED brightness
 *
 *  @param dev LED context
 *  @returns Brightness value
 */
int xpt_led_read_brightness(xpt_led_context dev);

/**
 * Read LED maximum brightness
 *
 *  @param dev LED context
 *  @returns Maximum brightness value
 */
int xpt_led_read_max_brightness(xpt_led_context dev);

/**
 * Set LED trigger
 *
 *  @param dev LED context
 *  @param trigger Type of trigger to set
 *  @returns Result of operation
 */
xpt_result_t xpt_led_set_trigger(xpt_led_context dev, const char* trigger);

/**
 * Clear active LED trigger
 *
 *  @param dev LED context
 *  @returns Result of operation
 */
xpt_result_t xpt_led_clear_trigger(xpt_led_context dev);

/**
 * Close LED file descriptors and free the context memory
 *
 *  @param dev LED context
 *  @returns Result of operation
 */
xpt_result_t xpt_led_close(xpt_led_context dev);

#ifdef __cplusplus
}
#endif
