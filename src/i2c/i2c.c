#include "i2c.h"
#include "xpt_internal.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/errno.h>
#if defined(MSYS)
#define __USE_LINUX_IOCTL_DEFS
#endif
#include <sys/ioctl.h>
#include "linux/i2c-dev.h"
#include <errno.h>
#include <string.h>

typedef union i2c_smbus_data_union {
    uint8_t byte;        ///< data byte
    unsigned short word; ///< data short word
    uint8_t block[I2C_SMBUS_BLOCK_MAX + 2];
    ///< block[0] is used for length and one more for PEC
} i2c_smbus_data_t;

typedef struct i2c_smbus_ioctl_data_struct {
    uint8_t read_write;     ///< operation direction
    uint8_t command;        ///< ioctl command
    int size;               ///< data size
    i2c_smbus_data_t* data; ///< data
} i2c_smbus_ioctl_data_t;


// static xpt_adv_func_t* func_table;

int xpt_i2c_smbus_access(int fh, uint8_t read_write, uint8_t command, int size, i2c_smbus_data_t* data)
{
    i2c_smbus_ioctl_data_t args;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;

    return ioctl(fh, I2C_SMBUS, &args);
}

static xpt_i2c_context xpt_i2c_init_internal(xpt_adv_func_t* advance_func, unsigned int bus)
{
    xpt_result_t status = XPT_SUCCESS;

    if (advance_func == NULL)
        return NULL;

    xpt_i2c_context dev = (xpt_i2c_context) calloc(1, sizeof(struct _i2c));
    if (dev == NULL) {
        syslog(LOG_CRIT, "i2c%i_init: Failed to allocate memory for context", bus);
        return NULL;
    }

    dev->advance_func = advance_func;
    dev->busnum = bus;

    if (IS_FUNC_DEFINED(dev, i2c_init_pre)) {
        status = advance_func->i2c_init_pre(bus);
        if (status != XPT_SUCCESS)
            goto init_internal_cleanup;
    }

    if (IS_FUNC_DEFINED(dev, i2c_init_bus_replace)) {
        status = dev->advance_func->i2c_init_bus_replace(dev);
        if (status != XPT_SUCCESS)
            goto init_internal_cleanup;
    } else {
        char filepath[32];
        snprintf(filepath, 32, "/dev/i2c-%u", bus);
        if ((dev->fh = open(filepath, O_RDWR)) < 1) {
            syslog(LOG_ERR, "i2c%i_init: Failed to open requested i2c port %s: %s", bus, filepath, strerror(errno));
            status = XPT_ERROR_INVALID_RESOURCE;
            goto init_internal_cleanup;
        }

        if (ioctl(dev->fh, I2C_FUNCS, &dev->funcs) < 0) {
            syslog(LOG_CRIT, "i2c%i_init: Failed to get I2C_FUNC map from device: %s", bus, strerror(errno));
            dev->funcs = 0;
        }
    }

    if (IS_FUNC_DEFINED(dev, i2c_init_post)) {
        status = dev->advance_func->i2c_init_post(dev);
        if (status != XPT_SUCCESS)
            goto init_internal_cleanup;
    }


init_internal_cleanup:
    if (status == XPT_SUCCESS) {
        return dev;
    } else {
        if (dev != NULL)
            free(dev);
        return NULL;
   }
}


xpt_i2c_context xpt_i2c_init(int bus)
{
    xpt_board_t* board = plat;
    if (board == NULL) {
        syslog(LOG_ERR, "i2c%i_init: Platform Not Initialised", bus);
        return NULL;
    }

    if (xpt_is_sub_platform_id(bus)) {
        syslog(LOG_NOTICE, "i2c%i_init: Using sub platform", bus);
        board = board->sub_platform;
        if (board == NULL) {
            syslog(LOG_ERR, "i2c%i_init: Sub platform Not Initialised", bus);
            return NULL;
        }
        bus = xpt_get_sub_platform_index(bus);
    }
    syslog(LOG_NOTICE, "i2c_init: Selected bus %d", bus);

    if (board->i2c_bus_count == 0) {
        syslog(LOG_ERR, "i2c_init: No i2c buses defined in platform");
        return NULL;
    }
    if (bus >= board->i2c_bus_count) {
        syslog(LOG_ERR, "i2c_init: i2c%i over i2c bus count", bus);
        return NULL;
    }

    if (board->i2c_bus[bus].bus_id == -1) {
        syslog(LOG_ERR, "Invalid i2c bus %i, moving to default i2c bus %i", bus, board->def_i2c_bus);
        bus = board->def_i2c_bus;
    }
    if (!board->no_bus_mux) {
        int pos = board->i2c_bus[bus].sda;
        if (pos >=0 && board->pins[pos].i2c.mux_total > 0) {
            if (xpt_setup_mux_mapped(board->pins[pos].i2c) != XPT_SUCCESS) {
                syslog(LOG_ERR, "i2c%i_init: Failed to set-up i2c sda multiplexer", bus);
                return NULL;
            }
        }

        pos = board->i2c_bus[bus].scl;
        if (pos >=0 && board->pins[pos].i2c.mux_total > 0) {
            if (xpt_setup_mux_mapped(board->pins[pos].i2c) != XPT_SUCCESS) {
                syslog(LOG_ERR, "i2c%i_init: Failed to set-up i2c scl multiplexer", bus);
                return NULL;
            }
        }
    }

    return xpt_i2c_init_internal(board->adv_func, (unsigned int) board->i2c_bus[bus].bus_id);
}


xpt_i2c_context xpt_i2c_init_raw(unsigned int bus)
{
    return xpt_i2c_init_internal(plat == NULL ? NULL : plat->adv_func, bus);
}


xpt_result_t xpt_i2c_frequency(xpt_i2c_context dev, xpt_i2c_mode_t mode)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: frequency: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, i2c_set_frequency_replace)) {
        return dev->advance_func->i2c_set_frequency_replace(dev, mode);
    }
    return XPT_ERROR_FEATURE_NOT_SUPPORTED;
}

int xpt_i2c_read(xpt_i2c_context dev, uint8_t* data, int length)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: read: context is invalid");
        return -1;
    }

    int bytes_read = 0;
    if (IS_FUNC_DEFINED(dev, i2c_read_replace)) {
        bytes_read = dev->advance_func->i2c_read_replace(dev, data, length);
    }
    else {
        bytes_read = read(dev->fh, data, length);
    }
    if (bytes_read == length) {
        return length;
    }

    return -1;
}

int xpt_i2c_read_byte(xpt_i2c_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: read_byte: context is invalid");
        return -1;
    }

    if (IS_FUNC_DEFINED(dev, i2c_read_byte_replace))
        return dev->advance_func->i2c_read_byte_replace(dev);
    i2c_smbus_data_t d;
    if (xpt_i2c_smbus_access(dev->fh, I2C_SMBUS_READ, I2C_NOCMD, I2C_SMBUS_BYTE, &d) < 0) {
        syslog(LOG_ERR, "i2c%i: read_byte: Access error: %s", dev->busnum, strerror(errno));
        return -1;
    }
    return 0x0FF & d.byte;
}

int xpt_i2c_read_byte_data(xpt_i2c_context dev, uint8_t command)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: read_byte_data: context is invalid");
        return -1;
    }

    if (IS_FUNC_DEFINED(dev, i2c_read_byte_data_replace))
        return dev->advance_func->i2c_read_byte_data_replace(dev, command);
    i2c_smbus_data_t d;
    if (xpt_i2c_smbus_access(dev->fh, I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA, &d) < 0) {
       syslog(LOG_ERR, "i2c%i: read_byte_data: Access error: %s", dev->busnum, strerror(errno));
       return -1;
    }
    return 0x0FF & d.byte;
}

int xpt_i2c_read_word_data(xpt_i2c_context dev, uint8_t command)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: read_word_data: context is invalid");
        return -1;
    }

    if (IS_FUNC_DEFINED(dev, i2c_read_word_data_replace))
        return dev->advance_func->i2c_read_word_data_replace(dev, command);
    i2c_smbus_data_t d;
    if (xpt_i2c_smbus_access(dev->fh, I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA, &d) < 0) {
        syslog(LOG_ERR, "i2c%i: read_word_data: Access error: %s", dev->busnum, strerror(errno));
        return -1;
    }
    return 0xFFFF & d.word;
}

int xpt_i2c_read_bytes_data(xpt_i2c_context dev, uint8_t command, uint8_t* data, int length)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: read_bytes_data: context is invalid");
        return -1;
    }

    if (IS_FUNC_DEFINED(dev, i2c_read_bytes_data_replace))
        return dev->advance_func->i2c_read_bytes_data_replace(dev, command, data, length);
    struct i2c_rdwr_ioctl_data d;
    struct i2c_msg m[2];

    m[0].addr = dev->addr;
    m[0].flags = 0x00;
    m[0].len = 1;
    m[0].buf = (char*) &command;
    m[1].addr = dev->addr;
    m[1].flags = I2C_M_RD;
    m[1].len = length;
    m[1].buf = (char*) data;

    d.msgs = m;
    d.nmsgs = 2;

    int ret = ioctl(dev->fh, I2C_RDWR, &d);

    if (ret < 0)
    {
        syslog(LOG_ERR, "i2c%i: read_bytes_data: Access error: %s", dev->busnum, strerror(errno));
        return -1;
    }
    return length;
}

xpt_result_t xpt_i2c_write(xpt_i2c_context dev, const uint8_t* data, int length)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: write: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, i2c_write_replace))
        return dev->advance_func->i2c_write_replace(dev, data, length);
    i2c_smbus_data_t d;
    int i;
    uint8_t command = data[0];

    data = &data[1];
    length = length - 1;
    if (length > I2C_SMBUS_I2C_BLOCK_MAX) {
        length = I2C_SMBUS_I2C_BLOCK_MAX;
    }

    for (i = 1; i <= length; i++) {
        d.block[i] = data[i - 1];
    }
    d.block[0] = length;

    if (xpt_i2c_smbus_access(dev->fh, I2C_SMBUS_WRITE, command, I2C_SMBUS_I2C_BLOCK_DATA, &d) < 0) {
        syslog(LOG_ERR, "i2c%i: write: Access error: %s", dev->busnum, strerror(errno));
        return XPT_ERROR_UNSPECIFIED;
    }
    return XPT_SUCCESS;
}

xpt_result_t xpt_i2c_write_byte(xpt_i2c_context dev, const uint8_t data)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: write_byte: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, i2c_write_byte_replace)) {
        return dev->advance_func->i2c_write_byte_replace(dev, data);
    } else {
        if (xpt_i2c_smbus_access(dev->fh, I2C_SMBUS_WRITE, data, I2C_SMBUS_BYTE, NULL) < 0) {
            syslog(LOG_ERR, "i2c%i: write_byte: Access error: %s", dev->busnum, strerror(errno));
            return XPT_ERROR_UNSPECIFIED;
        }
        return XPT_SUCCESS;
    }
}

xpt_result_t xpt_i2c_write_byte_data(xpt_i2c_context dev, const uint8_t data, const uint8_t command)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: write_byte_data: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, i2c_write_byte_data_replace))
        return dev->advance_func->i2c_write_byte_data_replace(dev, data, command);
    i2c_smbus_data_t d;
    d.byte = data;
    if (xpt_i2c_smbus_access(dev->fh, I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE_DATA, &d) < 0) {
        syslog(LOG_ERR, "i2c%i: write_byte_data: Access error: %s", dev->busnum, strerror(errno));
        return XPT_ERROR_UNSPECIFIED;
    }
    return XPT_SUCCESS;
}

xpt_result_t xpt_i2c_write_word_data(xpt_i2c_context dev, const uint16_t data, const uint8_t command)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: write_word_data: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, i2c_write_word_data_replace))
        return dev->advance_func->i2c_write_word_data_replace(dev, data, command);
    i2c_smbus_data_t d;
    d.word = data;
    if (xpt_i2c_smbus_access(dev->fh, I2C_SMBUS_WRITE, command, I2C_SMBUS_WORD_DATA, &d) < 0) {
        syslog(LOG_ERR, "i2c%i: write_word_data: Access error: %s", dev->busnum, strerror(errno));
        return XPT_ERROR_UNSPECIFIED;
    }
    return XPT_SUCCESS;
}

xpt_result_t xpt_i2c_address(xpt_i2c_context dev, uint8_t addr)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: address: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    dev->addr = (int) addr;
    if (IS_FUNC_DEFINED(dev, i2c_address_replace)) {
        return dev->advance_func->i2c_address_replace(dev, addr);
    } else {
        if (ioctl(dev->fh, I2C_SLAVE_FORCE, addr) < 0) {
            syslog(LOG_ERR, "i2c%i: address: Failed to set slave address %d: %s", dev->busnum, addr, strerror(errno));
            return XPT_ERROR_UNSPECIFIED;
        }
        return XPT_SUCCESS;
    }
}


xpt_result_t xpt_i2c_stop(xpt_i2c_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "i2c: stop: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, i2c_stop_replace)) {
        return dev->advance_func->i2c_stop_replace(dev);
    }

    close(dev->fh);
    free(dev);
    return XPT_SUCCESS;
}

