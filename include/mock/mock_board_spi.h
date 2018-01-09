#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "xpt_internal.h"

#define MOCK_SPI_DEFAULT_FREQ 4000000
#define MOCK_SPI_DEFAULT_MODE XPT_SPI_MODE0
#define MOCK_SPI_DEFAULT_LSBMODE 0
#define MOCK_SPI_DEFAULT_BIT_PER_WORD 8
// This is XORed with each byte/word of the transmitted message to get the received one
#define MOCK_SPI_REPLY_DATA_MODIFIER_BYTE 0xAB
#define MOCK_SPI_REPLY_DATA_MODIFIER_WORD 0xABBA

xpt_result_t
xpt_mock_spi_init_raw_replace(xpt_spi_context dev, unsigned int bus, unsigned int cs);

xpt_result_t
xpt_mock_spi_stop_replace(xpt_spi_context dev);

xpt_result_t
xpt_mock_spi_bit_per_word_replace(xpt_spi_context dev, unsigned int bits);

xpt_result_t
xpt_mock_spi_lsbmode_replace(xpt_spi_context dev, xpt_boolean_t lsb);

xpt_result_t
xpt_mock_spi_mode_replace(xpt_spi_context dev, xpt_spi_mode_t mode);

xpt_result_t
xpt_mock_spi_frequency_replace(xpt_spi_context dev, int hz);

int
xpt_mock_spi_write_replace(xpt_spi_context dev, uint8_t data);

int
xpt_mock_spi_write_word_replace(xpt_spi_context dev, uint16_t data);

xpt_result_t
xpt_mock_spi_transfer_buf_replace(xpt_spi_context dev, uint8_t* data, uint8_t* rxbuf, int length);

xpt_result_t
xpt_mock_spi_transfer_buf_word_replace(xpt_spi_context dev, uint16_t* data, uint16_t* rxbuf, int length);

#ifdef __cplusplus
}
#endif
