#pragma once

/** @file
 *
 * This file defines the basic shared types for libxpt
 * this file is different to common.h in that swig takes this as an input
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XPT supported platform types
 */
typedef enum {
    XPT_INTEL_GALILEO_GEN1 = 0,    /**< The Generation 1 Galileo platform (RevD) */
    XPT_INTEL_GALILEO_GEN2 = 1,    /**< The Generation 2 Galileo platform (RevG/H) */
    XPT_INTEL_EDISON_FAB_C = 2,    /**< The Intel Edison (FAB C) */
    XPT_INTEL_DE3815 = 3,          /**< The Intel DE3815 Baytrail NUC */
    XPT_INTEL_MINNOWBOARD_MAX = 4, /**< The Intel Minnow Board Max */
    XPT_RASPBERRY_PI = 5,          /**< The different Raspberry PI Models -like  A,B,A+,B+ */
    XPT_BEAGLEBONE = 6,            /**< The different BeagleBone Black Modes B/C */
    XPT_BANANA = 7,                /**< Allwinner A20 based Banana Pi and Banana Pro */
    XPT_INTEL_NUC5 = 8,            /**< The Intel 5th generations Broadwell NUCs */
    XPT_96BOARDS = 9,              /**< Linaro 96boards */
    XPT_INTEL_SOFIA_3GR = 10,      /**< The Intel SoFIA 3GR */
    XPT_INTEL_CHERRYHILLS = 11,    /**< The Intel Braswell Cherryhills */
    XPT_UP = 12,                   /**< The UP Board */
    XPT_INTEL_JOULE_EXPANSION = 13,/**< The Intel Joule Expansion Board */
#if __STDC_VERSION__ >= 199901L
    XPT_INTEL_GT_TUCHUCK = XPT_INTEL_JOULE_EXPANSION, // deprecated
#endif
    XPT_PHYBOARD_WEGA = 14,        /**< The phyBOARD-Wega */
    XPT_DE_NANO_SOC = 15,          /**< Terasic DE-Nano-SoC Board */
    XPT_UP2 = 16,                  /**< The UP^2 Board */
    XPT_MTK_LINKIT = 17,           /**< Mediatek MT7688 based Linkit boards */
    XPT_MTK_OMEGA2 = 18,           /**< MT7688 based Onion Omega2 board */

    // USB platform extenders start at 256
    XPT_FTDI_FT4222 = 256,         /**< FTDI FT4222 USB to i2c bridge */

    // contains bit 9 so is subplatform
    XPT_GROVEPI = 1024,            /**< GrovePi shield i2c bridge */
    XPT_GENERIC_FIRMATA = 1280,    /**< Firmata uart platform/bridge */

    XPT_ANDROID_PERIPHERALMANAGER = 95, /**< Android Things peripheral manager platform */
    XPT_MOCK_PLATFORM = 96,        /**< Mock platform, which requires no real hardware */
    XPT_JSON_PLATFORM = 97,        /**< User initialised platform from json */
    XPT_NULL_PLATFORM = 98,        /**< Platform with no capabilities that hosts a sub platform  */
    XPT_UNKNOWN_PLATFORM = 99      /**< An unknown platform type, typically will load INTEL_GALILEO_GEN1 */
} xpt_platform_t;

/**
 * Intel edison miniboard numbering enum
 */
typedef enum {
    XPT_INTEL_EDISON_MINIBOARD_J17_1 = 0,
    XPT_INTEL_EDISON_MINIBOARD_J17_5 = 4,
    XPT_INTEL_EDISON_MINIBOARD_J17_7 = 6,
    XPT_INTEL_EDISON_MINIBOARD_J17_8 = 7,
    XPT_INTEL_EDISON_MINIBOARD_J17_9 = 8,
    XPT_INTEL_EDISON_MINIBOARD_J17_10 = 9,
    XPT_INTEL_EDISON_MINIBOARD_J17_11 = 10,
    XPT_INTEL_EDISON_MINIBOARD_J17_12 = 11,
    XPT_INTEL_EDISON_MINIBOARD_J17_14 = 13,
    XPT_INTEL_EDISON_MINIBOARD_J18_1 = 14,
    XPT_INTEL_EDISON_MINIBOARD_J18_2 = 15,
    XPT_INTEL_EDISON_MINIBOARD_J18_6 = 19,
    XPT_INTEL_EDISON_MINIBOARD_J18_7 = 20,
    XPT_INTEL_EDISON_MINIBOARD_J18_8 = 21,
    XPT_INTEL_EDISON_MINIBOARD_J18_10 = 23,
    XPT_INTEL_EDISON_MINIBOARD_J18_11 = 24,
    XPT_INTEL_EDISON_MINIBOARD_J18_12 = 25,
    XPT_INTEL_EDISON_MINIBOARD_J18_13 = 26,
    XPT_INTEL_EDISON_MINIBOARD_J19_4 = 31,
    XPT_INTEL_EDISON_MINIBOARD_J19_5 = 32,
    XPT_INTEL_EDISON_MINIBOARD_J19_6 = 33,
    XPT_INTEL_EDISON_MINIBOARD_J19_8 = 35,
    XPT_INTEL_EDISON_MINIBOARD_J19_9 = 36,
    XPT_INTEL_EDISON_MINIBOARD_J19_10 = 37,
    XPT_INTEL_EDISON_MINIBOARD_J19_11 = 38,
    XPT_INTEL_EDISON_MINIBOARD_J19_12 = 39,
    XPT_INTEL_EDISON_MINIBOARD_J19_13 = 40,
    XPT_INTEL_EDISON_MINIBOARD_J19_14 = 41,
    XPT_INTEL_EDISON_MINIBOARD_J20_3 = 44,
    XPT_INTEL_EDISON_MINIBOARD_J20_4 = 45,
    XPT_INTEL_EDISON_MINIBOARD_J20_5 = 46,
    XPT_INTEL_EDISON_MINIBOARD_J20_6 = 47,
    XPT_INTEL_EDISON_MINIBOARD_J20_7 = 48,
    XPT_INTEL_EDISON_MINIBOARD_J20_8 = 49,
    XPT_INTEL_EDISON_MINIBOARD_J20_9 = 50,
    XPT_INTEL_EDISON_MINIBOARD_J20_10 = 51,
    XPT_INTEL_EDISON_MINIBOARD_J20_11 = 52,
    XPT_INTEL_EDISON_MINIBOARD_J20_12 = 53,
    XPT_INTEL_EDISON_MINIBOARD_J20_13 = 54,
    XPT_INTEL_EDISON_MINIBOARD_J20_14 = 55
} xpt_intel_edison_miniboard_t;

/**
 * Intel Edison raw GPIO numbering enum
 */
typedef enum {
    XPT_INTEL_EDISON_GP182 = 0,
    XPT_INTEL_EDISON_GP135 = 4,
    XPT_INTEL_EDISON_GP27 = 6,
    XPT_INTEL_EDISON_GP20 = 7,
    XPT_INTEL_EDISON_GP28 = 8,
    XPT_INTEL_EDISON_GP111 = 0,
    XPT_INTEL_EDISON_GP109 = 10,
    XPT_INTEL_EDISON_GP115 = 11,
    XPT_INTEL_EDISON_GP128 = 13,
    XPT_INTEL_EDISON_GP13 = 14,
    XPT_INTEL_EDISON_GP165 = 15,
    XPT_INTEL_EDISON_GP19 = 19,
    XPT_INTEL_EDISON_GP12 = 20,
    XPT_INTEL_EDISON_GP183 = 21,
    XPT_INTEL_EDISON_GP110 = 23,
    XPT_INTEL_EDISON_GP114 = 24,
    XPT_INTEL_EDISON_GP129 = 25,
    XPT_INTEL_EDISON_GP130 = 26,
    XPT_INTEL_EDISON_GP44 = 31,
    XPT_INTEL_EDISON_GP46 = 32,
    XPT_INTEL_EDISON_GP48 = 33,
    XPT_INTEL_EDISON_GP131 = 35,
    XPT_INTEL_EDISON_GP14 = 36,
    XPT_INTEL_EDISON_GP40 = 37,
    XPT_INTEL_EDISON_GP43 = 38,
    XPT_INTEL_EDISON_GP77 = 39,
    XPT_INTEL_EDISON_GP82 = 40,
    XPT_INTEL_EDISON_GP83 = 41,
    XPT_INTEL_EDISON_GP134 = 44,
    XPT_INTEL_EDISON_GP45 = 45,
    XPT_INTEL_EDISON_GP47 = 46,
    XPT_INTEL_EDISON_GP49 = 47,
    XPT_INTEL_EDISON_GP15 = 48,
    XPT_INTEL_EDISON_GP84 = 49,
    XPT_INTEL_EDISON_GP42 = 50,
    XPT_INTEL_EDISON_GP41 = 51,
    XPT_INTEL_EDISON_GP78 = 52,
    XPT_INTEL_EDISON_GP79 = 53,
    XPT_INTEL_EDISON_GP80 = 54,
    XPT_INTEL_EDISON_GP81 = 55
} xpt_intel_edison_t;

/**
* Raspberry PI Wiring compatible numbering enum
*/
typedef enum {
    XPT_RASPBERRY_WIRING_PIN8 = 3,
    XPT_RASPBERRY_WIRING_PIN9 = 5,
    XPT_RASPBERRY_WIRING_PIN7 = 7,
    XPT_RASPBERRY_WIRING_PIN15 = 8,
    XPT_RASPBERRY_WIRING_PIN16 = 10,
    XPT_RASPBERRY_WIRING_PIN0 = 11,
    XPT_RASPBERRY_WIRING_PIN1 = 12,
    XPT_RASPBERRY_WIRING_PIN2 = 13,
    XPT_RASPBERRY_WIRING_PIN3 = 15,
    XPT_RASPBERRY_WIRING_PIN4 = 16,
    XPT_RASPBERRY_WIRING_PIN5 = 18,
    XPT_RASPBERRY_WIRING_PIN12 = 19,
    XPT_RASPBERRY_WIRING_PIN13 = 21,
    XPT_RASPBERRY_WIRING_PIN6 = 22,
    XPT_RASPBERRY_WIRING_PIN14 = 23,
    XPT_RASPBERRY_WIRING_PIN10 = 24,
    XPT_RASPBERRY_WIRING_PIN11 = 26,
    XPT_RASPBERRY_WIRING_PIN17 = 29, // RPi B V2
    XPT_RASPBERRY_WIRING_PIN21 = 29,
    XPT_RASPBERRY_WIRING_PIN18 = 30, // RPi B V2
    XPT_RASPBERRY_WIRING_PIN19 = 31, // RPI B V2
    XPT_RASPBERRY_WIRING_PIN22 = 31,
    XPT_RASPBERRY_WIRING_PIN20 = 32, // RPi B V2
    XPT_RASPBERRY_WIRING_PIN26 = 32,
    XPT_RASPBERRY_WIRING_PIN23 = 33,
    XPT_RASPBERRY_WIRING_PIN24 = 35,
    XPT_RASPBERRY_WIRING_PIN27 = 36,
    XPT_RASPBERRY_WIRING_PIN25 = 37,
    XPT_RASPBERRY_WIRING_PIN28 = 38,
    XPT_RASPBERRY_WIRING_PIN29 = 40
} xpt_raspberry_wiring_t;

/**
 * XPT return codes
 */
typedef enum {
    XPT_SUCCESS = 0,                             /**< Expected response */
    XPT_ERROR_FEATURE_NOT_IMPLEMENTED = 1,       /**< Feature TODO */
    XPT_ERROR_FEATURE_NOT_SUPPORTED = 2,         /**< Feature not supported by HW */
    XPT_ERROR_INVALID_VERBOSITY_LEVEL = 3,       /**< Verbosity level wrong */
    XPT_ERROR_INVALID_PARAMETER = 4,             /**< Parameter invalid */
    XPT_ERROR_INVALID_HANDLE = 5,                /**< Handle invalid */
    XPT_ERROR_NO_RESOURCES = 6,                  /**< No resource of that type avail */
    XPT_ERROR_INVALID_RESOURCE = 7,              /**< Resource invalid */
    XPT_ERROR_INVALID_QUEUE_TYPE = 8,            /**< Queue type incorrect */
    XPT_ERROR_NO_DATA_AVAILABLE = 9,             /**< No data available */
    XPT_ERROR_INVALID_PLATFORM = 10,             /**< Platform not recognised */
    XPT_ERROR_PLATFORM_NOT_INITIALISED = 11,     /**< Board information not initialised */
    XPT_ERROR_UART_OW_SHORTED = 12,              /**< UART OW Short Circuit Detected*/
    XPT_ERROR_UART_OW_NO_DEVICES = 13,           /**< UART OW No devices detected */
    XPT_ERROR_UART_OW_DATA_ERROR = 14,           /**< UART OW Data/Bus error detected */

    XPT_ERROR_UNSPECIFIED = 99 /**< Unknown Error */
} xpt_result_t;

/**
 * Enum representing different possible modes for a pin.
 */
typedef enum {
    XPT_PIN_VALID = 0,     /**< Pin Valid */
    XPT_PIN_GPIO = 1,      /**< General Purpose IO */
    XPT_PIN_PWM = 2,       /**< Pulse Width Modulation */
    XPT_PIN_FAST_GPIO = 3, /**< Faster GPIO */
    XPT_PIN_SPI = 4,       /**< SPI */
    XPT_PIN_I2C = 5,       /**< I2C */
    XPT_PIN_AIO = 6,       /**< Analog in */
    XPT_PIN_UART = 7       /**< UART */
} xpt_pinmodes_t;

/**
 * Enum reprensenting different i2c speeds/modes
 */
typedef enum {
    XPT_I2C_STD = 0,  /**< up to 100Khz */
    XPT_I2C_FAST = 1, /**< up to 400Khz */
    XPT_I2C_HIGH = 2  /**< up to 3.4Mhz */
} xpt_i2c_mode_t;

/**
 * Enum representing different uart parity states
 */
typedef enum {
    XPT_UART_PARITY_NONE = 0,
    XPT_UART_PARITY_EVEN = 1,
    XPT_UART_PARITY_ODD = 2,
    XPT_UART_PARITY_MARK = 3,
    XPT_UART_PARITY_SPACE = 4
} xpt_uart_parity_t;

#ifdef __cplusplus
}
#endif
