#pragma once

/**
 * @file
 * @brief General Purpose IO
 *
 * Gpio is the General Purpose IO interface to libxpt. Its features depend on
 * the board type used, it can use gpiolibs (exported via a kernel module
 * through sysfs), or memory mapped IO via a /dev/uio device or /dev/mem
 * depending again on the board configuration.
 *
 * @snippet gpio_read6.c Interesting
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <pthread.h>
#include "common.h"

#if defined(SWIGJAVA) || defined(JAVACALLBACK)
#include <jni.h>
void xpt_java_isr_callback(void *args);
#endif

/**
 * Opaque pointer definition to the internal struct _gpio
 */
typedef struct _gpio* xpt_gpio_context;

/**
 * Gpio Output modes
 */
typedef enum {
    XPT_GPIO_STRONG = 0,   /**< Default. Strong high and low */
    XPT_GPIO_PULLUP = 1,   /**< Resistive High */
    XPT_GPIO_PULLDOWN = 2, /**< Resistive Low */
    XPT_GPIO_HIZ = 3       /**< High Z State */
} xpt_gpio_mode_t;

/**
 * Gpio Direction options
 */
typedef enum {
    XPT_GPIO_OUT = 0,      /**< Output. A Mode can also be set */
    XPT_GPIO_IN = 1,       /**< Input */
    XPT_GPIO_OUT_HIGH = 2, /**< Output. Init High */
    XPT_GPIO_OUT_LOW = 3   /**< Output. Init Low */
} xpt_gpio_dir_t;

/**
 * Gpio Edge types for interrupts
 */
typedef enum {
    XPT_GPIO_EDGE_NONE = 0,   /**< No interrupt on Gpio */
    XPT_GPIO_EDGE_BOTH = 1,   /**< Interrupt on rising & falling */
    XPT_GPIO_EDGE_RISING = 2, /**< Interrupt on rising only */
    XPT_GPIO_EDGE_FALLING = 3 /**< Interrupt on falling only */
} xpt_gpio_edge_t;

/**
 * Gpio input modes
 */
typedef enum {
    XPT_GPIO_ACTIVE_HIGH = 0, /**< Resistive High */
    XPT_GPIO_ACTIVE_LOW = 1,  /**< Resistive Low */
} xpt_gpio_input_mode_t;


/**
 * Gpio output driver modes
 */
typedef enum {
    XPT_GPIO_OPEN_DRAIN = 0, /**< Open Drain Configuration */
    XPT_GPIO_PUSH_PULL = 1,  /**< Push Pull Configuration */
} xpt_gpio_out_driver_mode_t;

/**
 * Initialise gpio_context, based on board number
 *
 *  @param pin Pin number read from the board, i.e IO3 is 3
 *  @returns gpio context or NULL
 */
xpt_gpio_context xpt_gpio_init(int pin);

/**
 * Initialise gpio context without any mapping to a pin
 *
 * @param gpiopin gpio pin as listed in SYSFS
 * @return gpio context or NULL
 */
xpt_gpio_context xpt_gpio_init_raw(int gpiopin);

/**
 * Set the edge mode on the gpio
 *
 * @param dev The Gpio context
 * @param mode The edge mode to set the gpio into
 * @return Result of operation
 */
xpt_result_t xpt_gpio_edge_mode(xpt_gpio_context dev, xpt_gpio_edge_t mode);

/**
 * Set an interrupt on pin
 *
 * @param dev The Gpio context
 * @param edge The edge mode to set the gpio into
 * @param fptr Function pointer to function to be called when interrupt is
 * triggered
 * @param args Arguments passed to the interrupt handler (fptr)
 * @return Result of operation
 */
xpt_result_t xpt_gpio_isr(xpt_gpio_context dev, xpt_gpio_edge_t edge, void (*fptr)(void*), void* args);

/**
 * Stop the current interrupt watcher on this Gpio, and set the Gpio edge mode
 * to XPT_GPIO_EDGE_NONE
 *
 * @param dev The Gpio context
 * @return Result of operation
 */
xpt_result_t xpt_gpio_isr_exit(xpt_gpio_context dev);

/**
 * Set Gpio Output Mode,
 *
 * @param dev The Gpio context
 * @param mode The Gpio Output Mode
 * @return Result of operation
 */
xpt_result_t xpt_gpio_mode(xpt_gpio_context dev, xpt_gpio_mode_t mode);

/**
 * Set Gpio direction
 *
 * @param dev The Gpio context
 * @param dir The direction of the Gpio
 * @return Result of operation
 */
xpt_result_t xpt_gpio_dir(xpt_gpio_context dev, xpt_gpio_dir_t dir);

/**
 * Read Gpio direction
 *
 * @param dev The Gpio context
 * @param dir The address where to store the Gpio direction
 * @return Result of operation
 */
xpt_result_t xpt_gpio_read_dir(xpt_gpio_context dev, xpt_gpio_dir_t *dir);

/**
 * Close the Gpio context
 * - Will free the memory for the context and unexport the Gpio
 *
 * @param dev The Gpio context
 * @return Result of operation
 */
xpt_result_t xpt_gpio_close(xpt_gpio_context dev);

/**
 * Read the Gpio value. This can be 0 or 1. A resonse of -1 means that there
 * was a fatal error.
 *
 * @param dev The Gpio context
 * @return Result of operation
 */
int xpt_gpio_read(xpt_gpio_context dev);

/**
 * Write to the Gpio Value.
 *
 * @param dev The Gpio context
 * @param value Integer value to write
 * @return Result of operation
 */
xpt_result_t xpt_gpio_write(xpt_gpio_context dev, int value);

/**
 * Change ownership of the context.
 *
 * @param dev The Gpio context
 * @param owner Does this context own the pin
 * @return Result of operation
 */
xpt_result_t xpt_gpio_owner(xpt_gpio_context dev, xpt_boolean_t owner);

/**
 * Enable using memory mapped io instead of sysfs
 *
 * @param dev The Gpio context
 * @param mmap Use mmap instead of sysfs
 * @return Result of operation
 */
xpt_result_t xpt_gpio_use_mmaped(xpt_gpio_context dev, xpt_boolean_t mmap);

/**
 * Get a pin number of the gpio, invalid will return -1
 *
 * @param dev The Gpio context
 * @return Pin number
 */
int xpt_gpio_get_pin(xpt_gpio_context dev);

/**
 * Get a gpio number as used within sysfs, invalid will return -1
 *
 * @param dev The Gpio context
 * @return gpio number
 */
int xpt_gpio_get_pin_raw(xpt_gpio_context dev);

/**
 * Set Gpio input mode
 *
 * @param dev The Gpio context
 * @param mode Mode to set input pin state
 * @return Result of operation
 */
xpt_result_t xpt_gpio_input_mode(xpt_gpio_context dev, xpt_gpio_input_mode_t mode);

/**
 * Set Gpio output driver mode. This is not a standard feature, it needs custom implementation for each board
 *
 * @param dev The Gpio context
 * @param mode Set output driver mode
 * @return Result of operation
 */
xpt_result_t xpt_gpio_out_driver_mode(xpt_gpio_context dev, xpt_gpio_out_driver_mode_t mode);

#ifdef __cplusplus
}
#endif
