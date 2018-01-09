#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "xpt_internal.h"

// Mock I2C device address
#define MOCK_I2C_DEV_ADDR 0x33
// Mock I2C device data registers block length in bytes. Our code assumes it's >= 1.
#define MOCK_I2C_DEV_DATA_LEN 10
// Initial value for each byte in the mock I2C device data registers
#define MOCK_I2C_DEV_DATA_INIT_BYTE 0xAB

xpt_result_t
xpt_mock_i2c_init_bus_replace(xpt_i2c_context dev);

xpt_result_t
xpt_mock_i2c_stop_replace(xpt_i2c_context dev);

xpt_result_t
xpt_mock_i2c_set_frequency_replace(xpt_i2c_context dev, xpt_i2c_mode_t mode);

xpt_result_t
xpt_mock_i2c_address_replace(xpt_i2c_context dev, uint8_t addr);

int
xpt_mock_i2c_read_replace(xpt_i2c_context dev, uint8_t* data, int length);

int
xpt_mock_i2c_read_byte_replace(xpt_i2c_context dev);

int
xpt_mock_i2c_read_byte_data_replace(xpt_i2c_context dev, uint8_t command);

int
xpt_mock_i2c_read_bytes_data_replace(xpt_i2c_context dev, uint8_t command, uint8_t* data, int length);

int
xpt_mock_i2c_read_word_data_replace(xpt_i2c_context dev, uint8_t command);

xpt_result_t
xpt_mock_i2c_write_replace(xpt_i2c_context dev, const uint8_t* data, int length);

xpt_result_t
xpt_mock_i2c_write_byte_replace(xpt_i2c_context dev, const uint8_t data);

xpt_result_t
xpt_mock_i2c_write_byte_data_replace(xpt_i2c_context dev, const uint8_t data, const uint8_t command);

xpt_result_t
xpt_mock_i2c_write_word_data_replace(xpt_i2c_context dev, const uint16_t data, const uint8_t command);

#ifdef __cplusplus
}
#endif
