#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <syslog.h>
#include <fnmatch.h>

#include "../api/xpt/common.h"
#include "xpt_internal_types.h"
#include "xpt_adv_func.h"
#include "xpt_lang_func.h"

extern xpt_board_t* plat;
extern char* platform_name;
#if !defined(PERIPHERALMAN)
extern xpt_iio_info_t* plat_iio;
#endif
extern xpt_lang_func_t* lang_func;

/**
 * Takes in pin information and sets up the multiplexors.
 *
 * @param meta
 * @return xpt result type indicating success of actions.
 */
xpt_result_t xpt_setup_mux_mapped(xpt_pin_t meta);

/**
 * runtime detect running x86 platform
 *
 * @return xpt_platform_t of the init'ed platform
 */
xpt_platform_t xpt_x86_platform();

/**
 * runtime detect running arm platforms
 *
 * @return xpt_platform_t of the init'ed platform
 */
xpt_platform_t xpt_arm_platform();

/**
 * runtime detect running mips platforms
 *
 * @return xpt_platform_t of the init'ed platform
 */
xpt_platform_t xpt_mips_platform();

/**
 * setup a mock platform
 *
 * @return xpt_platform_t of the init'ed platform
 */
xpt_platform_t xpt_mock_platform();

/**
 * runtime detect running usb platform extender
 *
 * @return xpt_platform_t of the detected platform extender
 */
xpt_platform_t xpt_usb_platform_extender(xpt_board_t* board);

/**
 * runtime detect iio subsystem
 *
 * @return xpt_result_t indicating success of iio detection
 */
xpt_result_t xpt_iio_detect();

/**
 * helper function to check if file exists
 *
 * @param filename to check
 * @return xpt_boolean_t boolean result.
 */
xpt_boolean_t xpt_file_exist(const char* filename);

/**
 * helper function to unglob filenames
 *
 * @param filename to unglob
 * @return char * with the existing filename matching the pattern of input. NULL if there is no
 *match. Caller must free result
 */
char* xpt_file_unglob(const char* filename);

/**
 * helper function to check if file contains a given text
 *
 * @param filename to check
 * @param content to check in file
 * @return xpt_boolean_t boolean result.
 */
xpt_boolean_t xpt_file_contains(const char* filename, const char* content);

/**
 * helper function to check if file contains a given text
 *
 * @param filename to check
 * @param content to check in file
 * @param content2 to check in same line of file
 * @return xpt_boolean_t boolean result.
 */
xpt_boolean_t xpt_file_contains_both(const char* filename, const char* content, const char* content2);

/**
 * helper function to find out if file that is targeted by a softlink
 * (partially) matches the given name
 *
 * @param filename of the softlink
 * @param (partial) filename that is matched with the filename of the link-targeted file
 * @return xpt_boolean_t true when targetname (partially) matches
 */
xpt_boolean_t xpt_link_targets(const char* filename, const char* targetname);

/**
 * helper function to find the first i2c bus containing devname starting from
 * i2c-n where n is startfrom
 *
 * @param device name to match
 * @param i2c-dev number to start search from
 * @return the matching i2c-dev bus id or -1
 */
int xpt_find_i2c_bus(const char* devname, int startfrom);

/**
 * helper function to wrap strtol for our basic usage
 *
 * @param string representing int
 * @param converted string
 * @return Result of the operation
 */
xpt_result_t xpt_atoi(char* intStr, int* value);

/**
 * helper function to find an i2c bus based on pci data
 *
 * @param pci_device
 * @param pci_id on pci_device
 * @param i2c adapter name & number
 * @return the matching i2c-dev bus id or -1
 */
int xpt_find_i2c_bus_pci(const char* pci_device, const char *pci_id, const char* adapter_name);

#if defined(IXPT)
/**
 * read Ixpt subplatform lock file, caller is responsible to free return
 * struct array
 *
 * @param ixpt lockfile location
 * @return the number of subplatforms added
 */
uint32_t xpt_add_from_lockfile(const char* ixpt_lock_file);

/**
 * Internal ixpt init function
 *
 * @return xpt_result_t indicating success
 */
xpt_result_t ixpt_init();
#endif

#ifdef __cplusplus
}
#endif
