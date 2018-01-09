#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "xpt_internal.h"

xpt_platform_t xpt_firmata_platform(xpt_board_t* board, const char* uart_dev);


#ifdef __cplusplus
}
#endif
