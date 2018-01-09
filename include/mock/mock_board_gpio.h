#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "xpt_internal.h"

xpt_result_t
xpt_mock_gpio_init_internal_replace(xpt_gpio_context dev, int pin);

xpt_result_t
xpt_mock_gpio_close_replace(xpt_gpio_context dev);

xpt_result_t
xpt_mock_gpio_dir_replace(xpt_gpio_context dev, xpt_gpio_dir_t dir);

xpt_result_t
xpt_mock_gpio_read_dir_replace(xpt_gpio_context dev, xpt_gpio_dir_t* dir);

int
xpt_mock_gpio_read_replace(xpt_gpio_context dev);

xpt_result_t
xpt_mock_gpio_write_replace(xpt_gpio_context dev, int value);

xpt_result_t
xpt_mock_gpio_edge_mode_replace(xpt_gpio_context dev, xpt_gpio_edge_t mode);

xpt_result_t
xpt_mock_gpio_isr_replace(xpt_gpio_context dev, xpt_gpio_edge_t mode, void (*fptr)(void*), void* args);

xpt_result_t
xpt_mock_gpio_isr_exit_replace(xpt_gpio_context dev);

xpt_result_t
xpt_mock_gpio_mode_replace(xpt_gpio_context dev, xpt_gpio_mode_t mode);

#ifdef __cplusplus
}
#endif
