/*
 * Author: Alex Tereschenko <alext.mkrs@gmail.com>
 * Copyright (c) 2016 Alex Tereschenko.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "xpt_internal.h"

// ASCII code for "Z", used as a basis for our mock reads
#define MOCK_UART_DATA_BYTE 0x5A

xpt_result_t
xpt_mock_uart_set_baudrate_replace(xpt_uart_context dev, unsigned int baud);

xpt_result_t
xpt_mock_uart_init_raw_replace(xpt_uart_context dev, const char* path);

xpt_result_t
xpt_mock_uart_flush_replace(xpt_uart_context dev);

xpt_result_t
xpt_mock_uart_sendbreak_replace(xpt_uart_context dev, int duration);

xpt_result_t
xpt_mock_uart_set_flowcontrol_replace(xpt_uart_context dev, xpt_boolean_t xonxoff, xpt_boolean_t rtscts);

xpt_result_t
xpt_mock_uart_set_mode_replace(xpt_uart_context dev, int bytesize, xpt_uart_parity_t parity, int stopbits);

xpt_result_t
xpt_mock_uart_set_non_blocking_replace(xpt_uart_context dev, xpt_boolean_t nonblock);

xpt_result_t
xpt_mock_uart_set_timeout_replace(xpt_uart_context dev, int read, int write, int interchar);

xpt_boolean_t
xpt_mock_uart_data_available_replace(xpt_uart_context dev, unsigned int millis);

int
xpt_mock_uart_write_replace(xpt_uart_context dev, const char* buf, size_t len);

int
xpt_mock_uart_read_replace(xpt_uart_context dev, char* buf, size_t len);

#ifdef __cplusplus
}
#endif
