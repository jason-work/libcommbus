#pragma once

/**
 * @file
 * @brief Pulse Width Modulation module
 *
 * PWM is the Pulse Width Modulation interface to libxpt. It allows the
 * generation of a signal on a pin. Some boards may have higher or lower levels
 * of resolution so make sure you check the board & pin you are using before
 * hand.
 *
 * @snippet cycle-pwm3.c Interesting
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <fcntl.h>

#include "common.h"

/** Xpt Pwm Context */
typedef struct _pwm* xpt_pwm_context;

/**
 * Initialise pwm_context, uses board mapping
 *
 * @param pin The PWM PIN
 * @return pwm context or NULL
 */
xpt_pwm_context xpt_pwm_init(int pin);

/**
 * Initialise pwm_context, raw mode
 *
 * @param chipid The chip inwhich the PWM is under in SYSFS
 * @param pin The PWM PIN.
 * @return pwm context or NULL
 */
xpt_pwm_context xpt_pwm_init_raw(int chipid, int pin);

/**
 * Set the output duty-cycle percentage, as a float
 *
 * @param dev The Pwm context to use
 * @param percentage A floating-point value representing percentage of output.
 *    The value should lie between 0.0f (representing on 0%) and 1.0f
 *    Values above or below this range will be set at either 0.0f or 1.0f
 * @return Result of operation
 */
xpt_result_t xpt_pwm_write(xpt_pwm_context dev, float percentage);

/**
 * Read the output duty-cycle percentage, as a float
 *
 * @param dev The Pwm context to use
 * @return percentage A floating-point value representing percentage of output.
 *    The value should lie between 0.0f (representing on 0%) and 1.0f
 *    Values above or below this range will be set at either 0.0f or 1.0f
 */
float xpt_pwm_read(xpt_pwm_context dev);

/**
 * Set the PWM period as seconds represented in a float
 *
 * @param dev The Pwm context to use
 * @param seconds Period represented as a float in seconds
 * @return Result of operation
 */
xpt_result_t xpt_pwm_period(xpt_pwm_context dev, float seconds);

/**
 * Set period, milliseconds.
 *
 * @param dev The Pwm context to use
 * @param ms Milliseconds for period
 * @return Result of operation
 */
xpt_result_t xpt_pwm_period_ms(xpt_pwm_context dev, int ms);

/**
 * Set period, microseconds
 *
 * @param dev The Pwm context to use
 * @param us Microseconds as period
 * @return Result of operation
 */
xpt_result_t xpt_pwm_period_us(xpt_pwm_context dev, int us);

/**
 * Set pulsewidth, As represnted by seconds in a (float)
 *
 * @param dev The Pwm context to use
 * @param seconds The duration of a pulse
 * @return Result of operation
 */
xpt_result_t xpt_pwm_pulsewidth(xpt_pwm_context dev, float seconds);

/**
 * Set pulsewidth, milliseconds
 *
 * @param dev The Pwm context to use
 * @param ms Milliseconds for pulsewidth
 * @return Result of operation
 */
xpt_result_t xpt_pwm_pulsewidth_ms(xpt_pwm_context dev, int ms);

/**
 * Set pulsewidth, microseconds
 *
 * @param dev The Pwm context to use
 * @param us Microseconds for pulsewidth
 * @return Result of operation
 */
xpt_result_t xpt_pwm_pulsewidth_us(xpt_pwm_context dev, int us);

/**
 * Set the enable status of the PWM pin. None zero will assume on with output being driven.
 *   and 0 will disable the output.
 *
 * @param dev The pwm context to use
 * @param enable Toggle status of pin
 * @return Result of operation.
 */
xpt_result_t xpt_pwm_enable(xpt_pwm_context dev, int enable);

/**
 * Change ownership of context
 *
 * @param dev the context
 * @param owner Ownership boolean
 * @return Result of operation
 */
xpt_result_t xpt_pwm_owner(xpt_pwm_context dev, xpt_boolean_t owner);

/**
 * Close and unexport the PWM pin
 *
 * @param dev The pwm context to use
 * @return Result of operation
 */
xpt_result_t xpt_pwm_close(xpt_pwm_context dev);

/**
 * Get the maximum pwm period in us
 *
 * @param dev The pwm context to use
 * @return max pwm in us
 */
int xpt_pwm_get_max_period(xpt_pwm_context dev);

/**
 * Get the minimum pwm period in us
 *
 * @param dev The pwm context to use
 * @return min pwm in us
 */
int xpt_pwm_get_min_period(xpt_pwm_context dev);

#ifdef __cplusplus
}
#endif
