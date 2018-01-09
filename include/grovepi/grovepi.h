#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"
#include "xpt_internal.h"

#define GROVEPI_ADDRESS     0x04
#define GROVEPI_REGISTER    0x01
#define GROVEPI_GPIO_READ   0x01
#define GROVEPI_GPIO_WRITE  0x02
#define GROVEPI_AIO_READ    0x03
#define GROVEPI_PWM         0x04
#define GROVEPI_GPIO_MODE   0x05
#define GROVEPI_FIRMWARE    0x08

xpt_platform_t xpt_grovepi_platform(xpt_board_t* board, const int i2c_bus);

#ifdef __cplusplus
}
#endif
