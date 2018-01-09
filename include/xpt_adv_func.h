#pragma once

#include "../api/xpt/common.h"
#include "../api/xpt.h"
#include "../api/xpt/types.h"

// FIXME: Nasty macro to test for presence of function in context structure function table
#define IS_FUNC_DEFINED(dev, func)   (dev != NULL && dev->advance_func != NULL && dev->advance_func->func != NULL)

typedef struct {
    xpt_result_t (*gpio_init_internal_replace) (xpt_gpio_context dev, int pin);
    xpt_result_t (*gpio_init_pre) (int pin);
    xpt_result_t (*gpio_init_post) (xpt_gpio_context dev);

    xpt_result_t (*gpio_close_pre) (xpt_gpio_context dev);
    xpt_result_t (*gpio_close_replace) (xpt_gpio_context dev);

    xpt_result_t (*gpio_mode_replace) (xpt_gpio_context dev, xpt_gpio_mode_t mode);
    xpt_result_t (*gpio_mode_pre) (xpt_gpio_context dev, xpt_gpio_mode_t mode);
    xpt_result_t (*gpio_mode_post) (xpt_gpio_context dev, xpt_gpio_mode_t mode);

    xpt_result_t (*gpio_edge_mode_replace) (xpt_gpio_context dev, xpt_gpio_edge_t mode);

    xpt_result_t (*gpio_dir_replace) (xpt_gpio_context dev, xpt_gpio_dir_t dir);
    xpt_result_t (*gpio_dir_pre) (xpt_gpio_context dev, xpt_gpio_dir_t dir);
    xpt_result_t (*gpio_dir_post) (xpt_gpio_context dev, xpt_gpio_dir_t dir);
    xpt_result_t (*gpio_read_dir_replace) (xpt_gpio_context dev, xpt_gpio_dir_t *dir);

    int (*gpio_read_replace) (xpt_gpio_context dev);
    xpt_result_t (*gpio_write_replace) (xpt_gpio_context dev, int value);
    xpt_result_t (*gpio_write_pre) (xpt_gpio_context dev, int value);
    xpt_result_t (*gpio_write_post) (xpt_gpio_context dev, int value);
    xpt_result_t (*gpio_mmap_setup) (xpt_gpio_context dev, xpt_boolean_t en);
    xpt_result_t (*gpio_interrupt_handler_init_replace) (xpt_gpio_context dev);
    xpt_result_t (*gpio_wait_interrupt_replace) (xpt_gpio_context dev);
    xpt_result_t (*gpio_isr_replace) (xpt_gpio_context dev, xpt_gpio_edge_t mode, void (*fptr)(void*), void* args);
    xpt_result_t (*gpio_isr_exit_replace) (xpt_gpio_context dev);
    xpt_result_t (*gpio_out_driver_mode_replace) (xpt_gpio_context dev, xpt_gpio_out_driver_mode_t mode);

    xpt_result_t (*i2c_init_pre) (unsigned int bus);
    xpt_result_t (*i2c_init_bus_replace) (xpt_i2c_context dev);
    xpt_i2c_context (*i2c_init_raw_replace) (unsigned int bus);
    xpt_result_t (*i2c_init_post) (xpt_i2c_context dev);
    xpt_result_t (*i2c_set_frequency_replace) (xpt_i2c_context dev, xpt_i2c_mode_t mode);
    xpt_result_t (*i2c_address_replace) (xpt_i2c_context dev, uint8_t addr);
    int (*i2c_read_replace) (xpt_i2c_context dev, uint8_t* data, int length);
    int (*i2c_read_byte_replace) (xpt_i2c_context dev);
    int (*i2c_read_byte_data_replace) (xpt_i2c_context dev, const uint8_t command);
    int (*i2c_read_word_data_replace) (xpt_i2c_context dev, const uint8_t command);
    int (*i2c_read_bytes_data_replace) (xpt_i2c_context dev, uint8_t command, uint8_t* data, int length);
    xpt_result_t (*i2c_write_replace) (xpt_i2c_context dev, const uint8_t* data, int length);
    xpt_result_t (*i2c_write_byte_replace) (xpt_i2c_context dev, uint8_t data);
    xpt_result_t (*i2c_write_byte_data_replace) (xpt_i2c_context dev, const uint8_t data, const uint8_t command);
    xpt_result_t (*i2c_write_word_data_replace) (xpt_i2c_context dev, const uint16_t data, const uint8_t command);
    xpt_result_t (*i2c_stop_replace) (xpt_i2c_context dev);

    xpt_result_t (*aio_init_internal_replace) (xpt_aio_context dev, int pin);
    xpt_result_t (*aio_close_replace) (xpt_aio_context dev);
    int (*aio_read_replace) (xpt_aio_context dev);
    xpt_result_t (*aio_get_valid_fp) (xpt_aio_context dev);
    xpt_result_t (*aio_init_pre) (unsigned int aio);
    xpt_result_t (*aio_init_post) (xpt_aio_context dev);

    xpt_pwm_context (*pwm_init_replace) (int pin);
    xpt_pwm_context (*pwm_init_internal_replace) (void* func_table, int pin);
    xpt_result_t (*pwm_init_raw_replace) (xpt_pwm_context dev, int pin);
    xpt_result_t (*pwm_init_pre) (int pin);
    xpt_result_t (*pwm_init_post) (xpt_pwm_context pwm);
    xpt_result_t (*pwm_period_replace) (xpt_pwm_context dev, int period);
    float (*pwm_read_replace) (xpt_pwm_context dev);
    xpt_result_t (*pwm_write_replace) (xpt_pwm_context dev, float duty);
    xpt_result_t (*pwm_write_pre) (xpt_pwm_context dev, float percentage);
    xpt_result_t (*pwm_enable_replace) (xpt_pwm_context dev, int enable);
    xpt_result_t (*pwm_enable_pre) (xpt_pwm_context dev, int enable);

    xpt_result_t (*spi_init_pre) (int bus);
    xpt_result_t (*spi_init_post) (xpt_spi_context spi);
    xpt_result_t (*spi_init_raw_replace) (xpt_spi_context spi, unsigned int bus, unsigned int cs);
    xpt_result_t (*spi_lsbmode_replace) (xpt_spi_context dev, xpt_boolean_t lsb);
    xpt_result_t (*spi_mode_replace) (xpt_spi_context dev, xpt_spi_mode_t mode);
    xpt_result_t (*spi_bit_per_word_replace) (xpt_spi_context dev, unsigned int bits);
    xpt_result_t (*spi_frequency_replace) (xpt_spi_context dev, int hz);
    xpt_result_t (*spi_transfer_buf_replace) (xpt_spi_context dev, uint8_t* data, uint8_t* rxbuf, int length);
    xpt_result_t (*spi_transfer_buf_word_replace) (xpt_spi_context dev, uint16_t* data, uint16_t* rxbuf, int length);
    int (*spi_write_replace) (xpt_spi_context dev, uint8_t data);
    int (*spi_write_word_replace) (xpt_spi_context dev, uint16_t data);
    xpt_result_t (*spi_stop_replace) (xpt_spi_context dev);

    xpt_result_t (*uart_init_pre) (int index);
    xpt_result_t (*uart_init_post) (xpt_uart_context uart);
    xpt_result_t (*uart_init_raw_replace) (xpt_uart_context dev, const char* path);
    xpt_result_t (*uart_flush_replace) (xpt_uart_context dev);
    xpt_result_t (*uart_sendbreak_replace) (xpt_uart_context dev, int duration);
    xpt_result_t (*uart_set_baudrate_replace) (xpt_uart_context dev, unsigned int baud);
    xpt_result_t (*uart_set_mode_replace) (xpt_uart_context dev, int bytesize, xpt_uart_parity_t parity, int stopbits);
    xpt_result_t (*uart_set_flowcontrol_replace) (xpt_uart_context dev, xpt_boolean_t xonxoff, xpt_boolean_t rtscts);
    xpt_result_t (*uart_set_timeout_replace) (xpt_uart_context dev, int read, int write, int interchar);
    xpt_result_t (*uart_set_non_blocking_replace) (xpt_uart_context dev, xpt_boolean_t nonblock);
    int (*uart_read_replace) (xpt_uart_context dev, char* buf, size_t len);
    int (*uart_write_replace)(xpt_uart_context dev, const char* buf, size_t len);
    xpt_boolean_t (*uart_data_available_replace) (xpt_uart_context dev, unsigned int millis);
} xpt_adv_func_t;
