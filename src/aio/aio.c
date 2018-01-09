#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "aio.h"
#include "xpt_internal.h"

#define DEFAULT_BITS 10

static int raw_bits;
static unsigned int shifter_value;
static float max_analog_value;

static xpt_result_t
aio_get_valid_fp(xpt_aio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "aio: get_valid_fp: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, aio_get_valid_fp)) {
        return dev->advance_func->aio_get_valid_fp(dev);
    }

    char file_path[64] = "";

    // Open file Analog device input channel raw voltage file for reading.
    snprintf(file_path, 64, "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw", dev->channel);

    dev->adc_in_fp = open(file_path, O_RDONLY);
    if (dev->adc_in_fp == -1) {
        syslog(LOG_ERR, "aio: Failed to open input raw file %s for reading!", file_path);
        return XPT_ERROR_INVALID_RESOURCE;
    }

    return XPT_SUCCESS;
}

static xpt_aio_context
xpt_aio_init_internal(xpt_adv_func_t* func_table, int aio, unsigned int channel)
{
    xpt_aio_context dev = calloc(1, sizeof(struct _aio));
    if (dev == NULL) {
        return NULL;
    }
    dev->advance_func = func_table;

    if (IS_FUNC_DEFINED(dev, aio_init_internal_replace)) {
        if (dev->advance_func->aio_init_internal_replace(dev, aio) == XPT_SUCCESS) {
            return dev;
        }
        free(dev);
        return NULL;
    }

    dev->channel = channel;

    // Open valid  analog input file and get the pointer.
    if (XPT_SUCCESS != aio_get_valid_fp(dev)) {
        free(dev);
        return NULL;
    }

    return dev;
}

xpt_aio_context
xpt_aio_init(unsigned int aio)
{
    xpt_board_t* board = plat;
    int pin;
    if (board == NULL) {
        syslog(LOG_ERR, "aio: Platform not initialised");
        return NULL;
    }
    if (xpt_is_sub_platform_id(aio)) {
        syslog(LOG_NOTICE, "aio: Using sub platform");
        board = board->sub_platform;
        if (board == NULL) {
            syslog(LOG_ERR, "aio: Sub platform Not Initialised");
            return NULL;
        }
        aio = xpt_get_sub_platform_index(aio);
    }

    // Some boards, like the BBB, don't have sequential AIO pins
    // They will have their own specific mapping to map aio -> pin
    if((board->aio_non_seq) && (aio < board->aio_count)){
        pin = board->aio_dev[aio].pin;
    } else {
        // aio are always past the gpio_count in the pin array
        pin = aio + board->gpio_count;
    }

    if (pin < 0 || pin >= board->phy_pin_count) {
        syslog(LOG_ERR, "aio: pin %i beyond platform definition", pin);
        return NULL;
    }
    if (aio >= board->aio_count) {
        syslog(LOG_ERR, "aio: requested channel out of range");
        return NULL;
    }
    if (board->pins[pin].capabilities.aio != 1) {
        syslog(LOG_ERR, "aio: pin %i not capable of aio", pin);
        return NULL;
    }
    if (board->pins[pin].aio.mux_total > 0) {
        if (xpt_setup_mux_mapped(board->pins[pin].aio) != XPT_SUCCESS) {
            syslog(LOG_ERR, "aio: unable to setup multiplexers for pin");
            return NULL;
        }
    }

    // Create ADC device connected to specified channel
    xpt_aio_context dev = xpt_aio_init_internal(board->adv_func, aio, board->pins[pin].aio.pinmap);
    if (dev == NULL) {
        syslog(LOG_ERR, "aio: Insufficient memory for specified input channel %d", aio);
        return NULL;
    }
    dev->value_bit = DEFAULT_BITS;

    if (IS_FUNC_DEFINED(dev, aio_init_pre)) {
        xpt_result_t pre_ret = (dev->advance_func->aio_init_pre(aio));
        if (pre_ret != XPT_SUCCESS) {
            free(dev);
            return NULL;
        }
    }

    if (IS_FUNC_DEFINED(dev, aio_init_post)) {
        xpt_result_t ret = dev->advance_func->aio_init_post(dev);
        if (ret != XPT_SUCCESS) {
            free(dev);
            return NULL;
        }
    }

    raw_bits = xpt_adc_raw_bits();

    if (raw_bits < dev->value_bit) {
        shifter_value = dev->value_bit - raw_bits;
        max_analog_value = ((1 << raw_bits) - 1) << shifter_value;
    } else {
        shifter_value = raw_bits - dev->value_bit;
        max_analog_value = ((1 << raw_bits) - 1) >> shifter_value;
    }

    return dev;
}

int
xpt_aio_read(xpt_aio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "aio: read: context is invalid");
        return -1;
    }

    if (IS_FUNC_DEFINED(dev, aio_read_replace)) {
        return dev->advance_func->aio_read_replace(dev);
    }

    char buffer[17];
    if (dev->adc_in_fp == -1) {
        if (aio_get_valid_fp(dev) != XPT_SUCCESS) {
            syslog(LOG_ERR, "aio: Failed to get to the device");
            return -1;
        }
    }

    lseek(dev->adc_in_fp, 0, SEEK_SET);
    if (read(dev->adc_in_fp, buffer, sizeof(buffer)) < 1) {
        syslog(LOG_ERR, "aio: Failed to read a sensible value");
    }
    // force NULL termination of string
    buffer[16] = '\0';
    lseek(dev->adc_in_fp, 0, SEEK_SET);

    errno = 0;
    char* end;
    unsigned int analog_value = (unsigned int) strtoul(buffer, &end, 10);
    if (end == &buffer[0]) {
        syslog(LOG_ERR, "aio: Value is not a decimal number");
        return -1;
    } else if (errno != 0) {
        syslog(LOG_ERR, "aio: Errno was set");
        return -1;
    }

    /* Adjust the raw analog input reading to supported resolution value*/
    if (raw_bits < dev->value_bit) {
        analog_value = analog_value << shifter_value;
    } else {
        analog_value = analog_value >> shifter_value;
    }

    return analog_value;
}

float
xpt_aio_read_float(xpt_aio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "aio: Device not valid");
        return -1.0;
    }

    unsigned int analog_value_int = xpt_aio_read(dev);

    return analog_value_int / max_analog_value;
}

xpt_result_t
xpt_aio_close(xpt_aio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "aio: close: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, aio_close_replace)) {
        return dev->advance_func->aio_close_replace(dev);
    }

    if (dev->adc_in_fp != -1) {
        close(dev->adc_in_fp);
    }

    free(dev);

    return XPT_SUCCESS;
}

xpt_result_t
xpt_aio_set_bit(xpt_aio_context dev, int bits)
{
    if (dev == NULL || bits < 1) {
        syslog(LOG_ERR, "aio: Device not valid");
        return XPT_ERROR_INVALID_RESOURCE;
    }
    dev->value_bit = bits;
    return XPT_SUCCESS;
}

int
xpt_aio_get_bit(xpt_aio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "aio: Device not valid");
        return 0;
    }
    return dev->value_bit;
}