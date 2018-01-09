#include "grovepi/grovepi.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static xpt_i2c_context grovepi_bus;
static int pwm_cache[10];

static int
xpt_grovepi_read_internal(int function, int pin)
{
    uint8_t data[5];
    uint8_t result[3];
    data[0] = GROVEPI_REGISTER;
    data[1] = function;
    data[2] = pin;
    data[3] = 0;
    data[4] = 0;
    if (xpt_i2c_write(grovepi_bus, data, 5) != XPT_SUCCESS) {
        syslog(LOG_WARNING, "grovepi: failed to write command to i2c bus /dev/i2c-%d", grovepi_bus->busnum);
        return -1;
    }
    if (xpt_i2c_write_byte(grovepi_bus, 1) != XPT_SUCCESS) {
        syslog(LOG_WARNING, "grovepi: failed to write to i2c bus /dev/i2c-%d", grovepi_bus->busnum);
        return -1;
    }
    if (function == GROVEPI_GPIO_READ) {
        if (xpt_i2c_read(grovepi_bus, result, 1) != 1) {
            syslog(LOG_WARNING, "grovepi: failed to read result from i2c bus /dev/i2c-%d", grovepi_bus->busnum);
            return -1;
        }
        return result[0];
    }
    if (function == GROVEPI_AIO_READ) {
        if (xpt_i2c_read(grovepi_bus, result, 3) != 3) {
            syslog(LOG_WARNING, "grovepi: failed to read result from i2c bus /dev/i2c-%d", grovepi_bus->busnum);
            return -1;
        }
        return (result[1] << 8) | result [2];
    }
    return -1;
}

static xpt_result_t
xpt_grovepi_write_internal(int function, int pin, int value)
{
    uint8_t data[5];
    data[0] = GROVEPI_REGISTER;
    data[1] = function;
    data[2] = pin;
    data[3] = value;
    data[4] = 0;
    if (xpt_i2c_write(grovepi_bus, data, 5) != XPT_SUCCESS) {
        syslog(LOG_WARNING, "grovepi: failed to write command to i2c bus /dev/i2c-%d", grovepi_bus->busnum);
        return XPT_ERROR_UNSPECIFIED;
    }
    return XPT_SUCCESS;
}

static xpt_result_t
xpt_grovepi_aio_init_internal_replace(xpt_aio_context dev, int aio)
{
    dev->channel = aio;
    return XPT_SUCCESS;
}

static int
xpt_grovepi_aio_read_replace(xpt_aio_context dev)
{
    return xpt_grovepi_read_internal(GROVEPI_AIO_READ, dev->channel);
}

static xpt_result_t
xpt_grovepi_gpio_init_internal_replace(xpt_gpio_context dev, int pin)
{
    dev->pin = pin;
    dev->phy_pin = pin;

    return XPT_SUCCESS;
}

static int
xpt_grovepi_gpio_read_replace(xpt_gpio_context dev)
{
    return xpt_grovepi_read_internal(GROVEPI_GPIO_READ, dev->pin);
}

static xpt_result_t
xpt_grovepi_gpio_write_replace(xpt_gpio_context dev, int write_value)
{
    return xpt_grovepi_write_internal(GROVEPI_GPIO_WRITE, dev->pin, write_value);
}

static xpt_result_t
xpt_grovepi_gpio_mode_replace(xpt_gpio_context dev, xpt_gpio_mode_t mode)
{
    return XPT_ERROR_FEATURE_NOT_IMPLEMENTED;
}

static xpt_result_t
xpt_grovepi_gpio_dir_replace(xpt_gpio_context dev, xpt_gpio_dir_t dir)
{
    return XPT_SUCCESS;
}

static xpt_result_t
xpt_grovepi_gpio_close_replace(xpt_gpio_context dev)
{
    free(dev);
    return XPT_SUCCESS;
}

static xpt_pwm_context
xpt_grovepi_pwm_init_internal_replace(void* func_table, int pin)
{
    xpt_pwm_context dev = (xpt_pwm_context) calloc(1, sizeof(struct _pwm));
    if (dev == NULL) {
        return NULL;
    }
    dev->pin = pin;
    dev->chipid = 512;
    dev->period = 2048000; // Locked, in ns
    dev->advance_func = (xpt_adv_func_t*) func_table;

    return dev;
}

static xpt_result_t
xpt_grovepi_pwm_write_replace(xpt_pwm_context dev, float percentage)
{
    int value = (int)((percentage - 1) / 8000);
    pwm_cache[dev->pin] = value;
    return xpt_grovepi_write_internal(GROVEPI_PWM, dev->pin, value);
}

static float
xpt_grovepi_pwm_read_replace(xpt_pwm_context dev)
{
    if (pwm_cache[dev->pin]) {
        return (pwm_cache[dev->pin] + 1) * 8000;
    }
    return 0;
}

static xpt_result_t
xpt_grovepi_pwm_enable_replace(xpt_pwm_context dev, int enable)
{
    if(!enable) {
        return xpt_grovepi_write_internal(GROVEPI_GPIO_WRITE, dev->pin, 0);
    } else {
        return xpt_grovepi_write_internal(GROVEPI_PWM, dev->pin, pwm_cache[dev->pin]);
    }
}

static xpt_result_t
xpt_grovepi_pwm_period_replace(xpt_pwm_context dev, int period)
{
    syslog(LOG_WARNING, "You cannot set period of a PWM pin with GrovePi\n");
    return XPT_ERROR_FEATURE_NOT_IMPLEMENTED;
}

xpt_platform_t
xpt_grovepi_platform(xpt_board_t* board, const int i2c_bus)
{
    xpt_board_t* b = (xpt_board_t*) calloc(1, sizeof(xpt_board_t));
    if (b == NULL) {
        return XPT_NULL_PLATFORM;
    }

    grovepi_bus = xpt_i2c_init(i2c_bus);
    if (grovepi_bus == NULL) {
        syslog(LOG_WARNING, "grovepi: Failed to initialize i2c bus %d", i2c_bus);
        free(b);
        return XPT_NULL_PLATFORM;
    }
    xpt_i2c_address(grovepi_bus, GROVEPI_ADDRESS);

    b->platform_name = "grovepi";
    b->platform_version = "1.2.7"; // TODO: add firmware query function
    b->platform_type = XPT_GROVEPI;
    b->gpio_count = 10;
    b->aio_count = 4;
    b->adc_supported = 10;
    b->phy_pin_count = 14;
    b->pwm_min_period = 2048;
    b->pwm_max_period = 2048;

    b->pins = (xpt_pininfo_t*) calloc(b->phy_pin_count, sizeof(xpt_pininfo_t));
    if (b->pins == NULL) {
        free(b);
        return XPT_NULL_PLATFORM;
    }

    strncpy(b->pins[0].name, "IO0", 8);
    b->pins[0].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 0, 0 };
    b->pins[0].gpio.pinmap = 0;
    strncpy(b->pins[1].name, "IO1", 8);
    b->pins[1].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 0, 0 };
    b->pins[1].gpio.pinmap = 1;
    strncpy(b->pins[2].name, "IO2", 8);
    b->pins[2].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 0, 0 };
    b->pins[2].gpio.pinmap = 2;
    strncpy(b->pins[3].name, "IO3", 8);
    b->pins[3].capabilities = (xpt_pincapabilities_t){ 1, 1, 1, 0, 0, 0, 0, 0 };
    b->pins[3].gpio.pinmap = 3;
    strncpy(b->pins[4].name, "IO4", 8);
    b->pins[4].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 0, 0 };
    b->pins[4].gpio.pinmap = 4;
    strncpy(b->pins[5].name, "IO5", 8);
    b->pins[5].capabilities = (xpt_pincapabilities_t){ 1, 1, 1, 0, 0, 0, 0, 0 };
    b->pins[5].gpio.pinmap = 5;
    strncpy(b->pins[6].name, "IO6", 8);
    b->pins[6].capabilities = (xpt_pincapabilities_t){ 1, 1, 1, 0, 0, 0, 0, 0 };
    b->pins[6].gpio.pinmap = 6;
    strncpy(b->pins[7].name, "IO7", 8);
    b->pins[7].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 0, 0 };
    b->pins[7].gpio.pinmap = 7;
    strncpy(b->pins[8].name, "IO8", 8);
    b->pins[8].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 0, 0 };
    b->pins[8].gpio.pinmap = 8;
    strncpy(b->pins[9].name, "IO9", 8);
    b->pins[9].capabilities = (xpt_pincapabilities_t){ 1, 1, 1, 0, 0, 0, 0, 0 };
    b->pins[9].gpio.pinmap = 9;
    strncpy(b->pins[10].name, "A0", 8);
    b->pins[10].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 1, 0 };
    b->pins[10].gpio.pinmap = 10;
    b->pins[10].aio.pinmap = 0;
    strncpy(b->pins[11].name, "A1", 8);
    b->pins[11].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 1, 0 };
    b->pins[11].gpio.pinmap = 11;
    b->pins[11].aio.pinmap = 1;
    strncpy(b->pins[12].name, "A2", 8);
    b->pins[12].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 1, 0 };
    b->pins[12].gpio.pinmap = 12;
    b->pins[12].aio.pinmap = 2;
    strncpy(b->pins[13].name, "A3", 8);
    b->pins[13].capabilities = (xpt_pincapabilities_t){ 1, 1, 0, 0, 0, 0, 1, 0 };
    b->pins[13].gpio.pinmap = 13;
    b->pins[13].aio.pinmap = 3;

    b->adv_func = (xpt_adv_func_t*) calloc(1, sizeof(xpt_adv_func_t));
    if (b->adv_func == NULL) {
        free(b->pins);
        free(b);
        return XPT_NULL_PLATFORM;
    }

    b->adv_func->gpio_init_internal_replace = &xpt_grovepi_gpio_init_internal_replace;
    b->adv_func->gpio_mode_replace = &xpt_grovepi_gpio_mode_replace;
    b->adv_func->gpio_dir_replace = &xpt_grovepi_gpio_dir_replace;
    //TODO: add interrupt support
    //b->adv_func->gpio_edge_mode_replace = &xpt_grovepi_gpio_edge_mode_replace;
    //b->adv_func->gpio_interrupt_handler_init_replace = &xpt_grovepi_gpio_interrupt_handler_init_replace;
    //b->adv_func->gpio_wait_interrupt_replace = &xpt_grovepi_gpio_wait_interrupt_replace;
    b->adv_func->gpio_read_replace = &xpt_grovepi_gpio_read_replace;
    b->adv_func->gpio_write_replace = &xpt_grovepi_gpio_write_replace;
    b->adv_func->gpio_close_replace = &xpt_grovepi_gpio_close_replace;

    b->adv_func->aio_init_internal_replace = &xpt_grovepi_aio_init_internal_replace;
    b->adv_func->aio_read_replace = &xpt_grovepi_aio_read_replace;

    b->adv_func->pwm_init_internal_replace = &xpt_grovepi_pwm_init_internal_replace;
    b->adv_func->pwm_write_replace = &xpt_grovepi_pwm_write_replace;
    b->adv_func->pwm_read_replace = &xpt_grovepi_pwm_read_replace;
    b->adv_func->pwm_enable_replace = &xpt_grovepi_pwm_enable_replace;
    b->adv_func->pwm_period_replace = &xpt_grovepi_pwm_period_replace;

    board->sub_platform = b;

    return b->platform_type;
}
