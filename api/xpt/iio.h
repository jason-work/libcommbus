#pragma once

#include "common.h"
#include "iio_kernel_headers.h"

/** Xpt Iio Channels */
typedef struct {
    /** Channel index */
    int index;
    /** Channel enabled/disabled */
    int enabled;
    /** Channel type */
    char* type;
    /** Channel endianes */
    xpt_boolean_t lendian;
    /** Channel signed */
    int signedd;
    /** Channel offset */
    unsigned int offset;
    /** Channel mask */
    uint64_t mask;
    /** Channel used bits */
    unsigned int bits_used;
    /** Channel bytes */
    unsigned int bytes;
    /** Channel shift */
    unsigned int shift;
    /** Channel location */
    unsigned int location;
} xpt_iio_channel;

/** Xpt Iio Event */
typedef struct {
    /** Event name */
    char* name;
    /** Event enabled/disabled */
    int enabled;
} xpt_iio_event;

/**
 * @file
 * @brief iio
 *
 * An iio context represents an IIO device
 *
 * @snippet iio_driver.c Interesting
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>

#include "common.h"

/**
 * Opaque pointer definition to the internal struct _iio
 */
typedef struct _iio* xpt_iio_context;

/**
 * Initialise iio context
 *
 * @param device iio device to use
 * @return i2c context or NULL
 */
xpt_iio_context xpt_iio_init(int device);

/**
 * Trigger buffer
 *
 * @param dev The iio context
 * @param fptr Callback
 * @param args Arguments
 * @return Result of operation
 */
xpt_result_t xpt_iio_trigger_buffer(xpt_iio_context dev, void (*fptr)(char*, void*), void* args);

/**
 * Get device name
 *
 * @param dev The iio context
 * @return Name of the device
 */
const char* xpt_iio_get_device_name(xpt_iio_context dev);

/**
 * Get device number
 *
 * @param name Name of the device
 * @return Device Number
 */
int xpt_iio_get_device_num_by_name(const char* name);

/**
 * Read size
 *
 * @param dev The iio context
 * @return Size
 */
int xpt_iio_read_size(xpt_iio_context dev);

/**
 * Get channels
 *
 * @param dev The iio context
 * @return Channels
 */
xpt_iio_channel* xpt_iio_get_channels(xpt_iio_context dev);

/**
 * Get channels count
 *
 * @param dev The iio context
 * @return Channels count
 */
int xpt_iio_get_channel_count(xpt_iio_context dev);

/**
 * Read float from file
 *
 * @param dev The iio context
 * @param filename Filename
 * @param data Data
 * @return Result of operation
 */
xpt_result_t xpt_iio_read_float(xpt_iio_context dev, const char* filename, float* data);

/**
 * Read int from file
 *
 * @param dev The iio context
 * @param filename Filename
 * @param data Data
 * @return Result of operation
 */
xpt_result_t xpt_iio_read_int(xpt_iio_context dev, const char* filename, int* data);

/**
 * Read String from file
 *
 * @param dev The iio context
 * @param filename Filename
 * @param data Data
 * @param max_len Max lenght to read
 * @return Result of operation
 */
xpt_result_t xpt_iio_read_string(xpt_iio_context dev, const char* filename, char* data, int max_len);

/**
 * Write float
 *
 * @param dev The iio context
 * @param attr_chan Channel attributes
 * @param data Float to write
 * @return Result of operation
 */
xpt_result_t xpt_iio_write_float(xpt_iio_context dev, const char* attr_chan, const float data);

/**
 * Write int
 *
 * @param dev The iio context
 * @param attr_chan Channel attributes
 * @param data Int to write
 * @return Result of operation
 */
xpt_result_t xpt_iio_write_int(xpt_iio_context dev, const char* attr_chan, const int data);

/**
 * Write string
 *
 * @param dev The iio context
 * @param attr_chan Channel attributes
 * @param data String to write
 * @return Result of operation
 */
xpt_result_t xpt_iio_write_string(xpt_iio_context dev, const char* attr_chan, const char* data);

/**
 * Get channel data
 *
 * @param dev The iio context
 * @return Result of operation
 */
xpt_result_t xpt_iio_get_channel_data(xpt_iio_context dev);

/**
 * Get event data
 *
 * @param dev The iio context
 * @return Result of operation
 */
xpt_result_t xpt_iio_get_event_data(xpt_iio_context dev);

/**
 * Event poll
 *
 * @param dev The iio context
 * @param data Data
 * @return Result of operation
 */
xpt_result_t xpt_iio_event_poll(xpt_iio_context dev, struct iio_event_data* data);

/**
 * Setup event callback
 *
 * @param dev The iio context
 * @param fptr Callback
 * @param args Arguments
 * @return Result of operation
 */
xpt_result_t
xpt_iio_event_setup_callback(xpt_iio_context dev, void (*fptr)(struct iio_event_data* data, void* args), void* args);

/**
 * Extract event
 *
 * @param event Event
 * @param chan_type Channel type
 * @param modifier Modifier
 * @param type Type
 * @param direction Direction
 * @param channel Channel
 * @param channel2 Channel2
 * @param different Different
 * @return Result of operation
 */
xpt_result_t xpt_iio_event_extract_event(struct iio_event_data* event,
                                           int* chan_type,
                                           int* modifier,
                                           int* type,
                                           int* direction,
                                           int* channel,
                                           int* channel2,
                                           int* different);

/**
 * Get mount matrix
 * @param dev The iio context
 * @param sysfs_name Sysfs name
 * @param mm Matrix
 * @return Result of operation
 */
xpt_result_t xpt_iio_get_mount_matrix(xpt_iio_context dev, const char *sysfs_name, float mm[9]);

/**
 * Create trigger
 *
 * @param dev The iio context
 * @param trigger Trigger name
 * @return Result of operation
 */
xpt_result_t xpt_iio_create_trigger(xpt_iio_context dev, const char* trigger);

/**
 * Update channels
 *
 * @param dev The iio context
 * @return Result of operation
 */
xpt_result_t xpt_iio_update_channels(xpt_iio_context dev);

/**
 * De-inits an xpt_iio_context device
 *
 * @param dev The iio context
 * @return Result of operation
 */
xpt_result_t xpt_iio_close(xpt_iio_context dev);

#ifdef __cplusplus
}
#endif
