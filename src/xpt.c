#define _GNU_SOURCE
#if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 600
#define _XOPEN_SOURCE 600 /* Get nftw() and S_IFSOCK declarations */
#endif

#include <stddef.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <pwd.h>
#if !defined(PERIPHERALMAN)
#include <glob.h>
#include <ftw.h>
#endif
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>


#if defined(IXPT)
#include <json-c/json.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#include "xpt_internal.h"
#include "firmata/firmata_xpt.h"
#include "grovepi/grovepi.h"
#include "gpio.h"
#include "version.h"
#include "i2c.h"
#include "pwm.h"
#include "aio.h"
#include "spi.h"
#include "uart.h"

#if defined(PERIPHERALMAN)
#include "peripheralmanager/peripheralman.h"
#else
#define IIO_DEVICE_WILDCARD "iio:device*"

xpt_iio_info_t* plat_iio = NULL;

static int num_i2c_devices = 0;
static int num_iio_devices = 0;
#endif

xpt_board_t* plat = NULL;
xpt_lang_func_t* lang_func = NULL;

char* platform_name = NULL;

const char*
xpt_get_version()
{
    return gVERSION;
}

xpt_result_t
xpt_set_log_level(int level)
{
    if (level <= 7 && level >= 0) {
        setlogmask(LOG_UPTO(level));
        syslog(LOG_DEBUG, "Loglevel %d is set", level);
        return XPT_SUCCESS;
    }
    syslog(LOG_NOTICE, "Invalid loglevel %d requested", level);
    return XPT_ERROR_INVALID_PARAMETER;
}

/**
 * Whilst the actual xpt init function is now called ixpt_init, it's only
 * callable externally if IXPT is enabled
 */
xpt_result_t
ixpt_init()
{
    if (plat != NULL) {
        return XPT_SUCCESS;
    }
    char* env_var;
    xpt_platform_t platform_type = XPT_NULL_PLATFORM;
    uid_t proc_euid = geteuid();
    struct passwd* proc_user = getpwuid(proc_euid);

#ifdef DEBUG
    setlogmask(LOG_UPTO(LOG_DEBUG));
#else
    setlogmask(LOG_UPTO(LOG_NOTICE));
#endif

    openlog("libxpt", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_NOTICE, "libxpt version %s initialised by user '%s' with EUID %d",
           xpt_get_version(), (proc_user != NULL) ? proc_user->pw_name : "<unknown>", proc_euid);

    // Check to see if the enviroment variable has been set
    env_var = getenv(XPT_JSONPLAT_ENV_VAR);
    if (env_var != NULL) {
        // We only care about success, the init will write to syslog if things went wrong
        switch (xpt_init_json_platform(env_var)) {
            case XPT_SUCCESS:
                platform_type = plat->platform_type;
                break;
            default:
                syslog(LOG_NOTICE, "libxpt was unable to initialise a platform from json");
        }
    }

    // Not an else because if the env var didn't load what we wanted maybe we can still load something
    if (platform_type == XPT_NULL_PLATFORM) {
#if defined(X86PLAT)
        // Use runtime x86 platform detection
        platform_type = xpt_x86_platform();
#elif defined(ARMPLAT)
        // Use runtime ARM platform detection
        platform_type = xpt_arm_platform();
#elif defined(MIPSPLAT)
        // Use runtime ARM platform detection
        platform_type = xpt_mips_platform();
#elif defined(MOCKPLAT)
        // Use mock platform
        platform_type = xpt_mock_platform();
#elif defined(PERIPHERALMAN)
        // Use peripheralmanager
        platform_type = xpt_peripheralman_platform();
#else
#error xpt_ARCH NOTHING
#endif
    }

    if (plat != NULL) {
        plat->platform_type = platform_type;
    } else {
        platform_name = NULL;
    }

    // Create null base platform if one doesn't already exist
    if (plat == NULL) {
        plat = (xpt_board_t*) calloc(1, sizeof(xpt_board_t));
        if (plat != NULL) {
            plat->platform_type = XPT_NULL_PLATFORM;
            plat->platform_name = "Unknown platform";
        }
    }

#if defined(USBPLAT)
    // Now detect sub platform, note this is not an else since we could be in
    // an error case and fall through to XPT_ERROR_PLATFORM_NOT_INITIALISED
    if (plat != NULL) {
        xpt_platform_t usb_platform_type = xpt_usb_platform_extender(plat);
        // if we have no known platform just replace usb platform with platform
        if (plat->platform_type == XPT_UNKNOWN_PLATFORM && usb_platform_type != XPT_UNKNOWN_PLATFORM) {
            plat->platform_type = usb_platform_type;
        }
    }
    if (plat == NULL) {
        printf("xpt: FATAL error, failed to initialise platform\n");
        return XPT_ERROR_PLATFORM_NOT_INITIALISED;
    }
#endif

#if defined(IXPT)
    const char* subplatform_lockfile = "/tmp/ixpt.lock";
    xpt_add_from_lockfile(subplatform_lockfile);
#endif

#if !defined(PERIPHERALMAN)
    // Look for IIO devices
    xpt_iio_detect();

    if (plat != NULL) {
        int length = strlen(plat->platform_name) + 1;
        if (xpt_has_sub_platform()) {
            // Account for ' + ' chars
            length += strlen(plat->sub_platform->platform_name) + 3;
        }
        platform_name = calloc(length, sizeof(char));
        if (xpt_has_sub_platform()) {
            snprintf(platform_name, length, "%s + %s", plat->platform_name, plat->sub_platform->platform_name);
        } else {
            strncpy(platform_name, plat->platform_name, length);
        }
    }
#endif

    lang_func = (xpt_lang_func_t*) calloc(1, sizeof(xpt_lang_func_t));
    if (lang_func == NULL) {
        return XPT_ERROR_NO_RESOURCES;
    }

    syslog(LOG_NOTICE, "libxpt initialised for platform '%s' of type %d", xpt_get_platform_name(), xpt_get_platform_type());
    return XPT_SUCCESS;
}

#if (defined SWIGPYTHON) || (defined SWIG)
xpt_result_t
#else
xpt_result_t __attribute__((constructor))
#endif
xpt_init()
{
    if (plat != NULL) {
        return XPT_SUCCESS;
    } else {
        return ixpt_init();
    }
}

void
xpt_deinit()
{
    if (plat != NULL) {
        if (plat->pins != NULL) {
            free(plat->pins);
        }
        if (plat->adv_func != NULL) {
            free(plat->adv_func);
        }
        xpt_board_t* sub_plat = plat->sub_platform;
        if (sub_plat != NULL) {
            if (sub_plat->pins != NULL) {
                free(sub_plat->pins);
            }
            if (sub_plat->adv_func != NULL) {
                free(sub_plat->adv_func);
            }
            free(sub_plat);
        }
#if defined(JSONPLAT)
        if (plat->platform_type == XPT_JSON_PLATFORM) {
            // Free the platform name
            free(plat->platform_name);
            plat->platform_name = NULL;

            int i = 0;
            // Free the UART device path
            for (i = 0; i < plat->uart_dev_count; i++) {
                if (plat->uart_dev[i].device_path != NULL) {
                    free(plat->uart_dev[i].device_path);
                }
            }
        }
#endif
        free(plat);
        plat = NULL;

        if (lang_func != NULL) {
            free(lang_func);
            lang_func = NULL;
        }

        if (platform_name != NULL) {
            free(platform_name);
            platform_name = NULL;
        }
    }
#if !defined(PERIPHERALMAN)
    if (plat_iio != NULL) {
        free(plat_iio);
        plat_iio = NULL;
    }
#else
    pman_xpt_deinit();
#endif
    closelog();
}

int
xpt_set_priority(const int priority)
{
    struct sched_param sched_s;

    memset(&sched_s, 0, sizeof(struct sched_param));
    if (priority > sched_get_priority_max(SCHED_RR)) {
        sched_s.sched_priority = sched_get_priority_max(SCHED_RR);
    } else {
        sched_s.sched_priority = priority;
    }

    return sched_setscheduler(0, SCHED_RR, &sched_s);
}

#if !defined(PERIPHERALMAN)
static int
xpt_count_iio_devices(const char* path, const struct stat* sb, int flag, struct FTW* ftwb)
{
    // we are only interested in files with specific names
    if (fnmatch(IIO_DEVICE_WILDCARD, basename(path), 0) == 0) {
        num_iio_devices++;
    }
    return 0;
}

xpt_result_t
xpt_iio_detect()
{
    plat_iio = (xpt_iio_info_t*) calloc(1, sizeof(xpt_iio_info_t));
    plat_iio->iio_device_count = num_iio_devices;
    // Now detect IIO devices, linux only
    // find how many iio devices we have if we haven't already
    if (num_iio_devices == 0) {
        if (nftw("/sys/bus/iio/devices", &xpt_count_iio_devices, 20, FTW_PHYS) == -1) {
            return XPT_ERROR_UNSPECIFIED;
        }
    }
    char name[64], filepath[64];
    int fd, len, i;
    plat_iio->iio_device_count = num_iio_devices;
    plat_iio->iio_devices = calloc(num_iio_devices, sizeof(struct _iio));
    struct _iio* device;
    for (i=0; i < num_iio_devices; i++) {
        device = &plat_iio->iio_devices[i];
        device->num = i;
        snprintf(filepath, 64, "/sys/bus/iio/devices/iio:device%d/name", i);
        fd = open(filepath, O_RDONLY);
        if (fd != -1) {
            len = read(fd, &name, 64);
            if (len > 1) {
                // remove any trailing CR/LF symbols
                name[strcspn(name, "\r\n")] = '\0';
                len = strlen(name);
                // use strndup
                device->name = malloc((sizeof(char) * len) + sizeof(char));
                strncpy(device->name, name, len+1);
            }
            close(fd);
        }
    }
    return XPT_SUCCESS;
}

xpt_result_t
xpt_setup_mux_mapped(xpt_pin_t meta)
{
    unsigned int mi;
    xpt_result_t ret;
    xpt_gpio_context mux_i = NULL;
    // avoids the unsigned comparison and we should never have a pin that is UINT_MAX!
    unsigned int last_pin = UINT_MAX;

    for (mi = 0; mi < meta.mux_total; mi++) {

        switch(meta.mux[mi].pincmd) {
            case PINCMD_UNDEFINED:              // used for backward compatibility
                if(meta.mux[mi].pin != last_pin) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    mux_i = xpt_gpio_init_raw(meta.mux[mi].pin);
                    if (mux_i == NULL) return XPT_ERROR_INVALID_HANDLE;
                    last_pin = meta.mux[mi].pin;
                }
                // this function will sometimes fail, however this is not critical as
                // long as the write succeeds - Test case galileo gen2 pin2
                xpt_gpio_dir(mux_i, XPT_GPIO_OUT);
                ret = xpt_gpio_write(mux_i, meta.mux[mi].value);
                if(ret != XPT_SUCCESS) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    return XPT_ERROR_INVALID_RESOURCE;
                }
                break;

            case PINCMD_SET_VALUE:
                if(meta.mux[mi].pin != last_pin) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    mux_i = xpt_gpio_init_raw(meta.mux[mi].pin);
                    if (mux_i == NULL) return XPT_ERROR_INVALID_HANDLE;
                    last_pin = meta.mux[mi].pin;
                }

                ret = xpt_gpio_write(mux_i, meta.mux[mi].value);

                if(ret != XPT_SUCCESS) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    return XPT_ERROR_INVALID_RESOURCE;
                }
                break;

            case PINCMD_SET_DIRECTION:
                if(meta.mux[mi].pin != last_pin) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    mux_i = xpt_gpio_init_raw(meta.mux[mi].pin);
                    if (mux_i == NULL) return XPT_ERROR_INVALID_HANDLE;
                    last_pin = meta.mux[mi].pin;
                }

                ret = xpt_gpio_dir(mux_i, meta.mux[mi].value);

                if(ret != XPT_SUCCESS) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    return XPT_ERROR_INVALID_RESOURCE;
                }
                break;

            case PINCMD_SET_IN_VALUE:
                if(meta.mux[mi].pin != last_pin) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    mux_i = xpt_gpio_init_raw(meta.mux[mi].pin);
                    if (mux_i == NULL) return XPT_ERROR_INVALID_HANDLE;
                    last_pin = meta.mux[mi].pin;
                }

                ret = xpt_gpio_dir(mux_i, XPT_GPIO_IN);

                if(ret == XPT_SUCCESS)
                    ret = xpt_gpio_write(mux_i, meta.mux[mi].value);

                if(ret != XPT_SUCCESS) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    return XPT_ERROR_INVALID_RESOURCE;
                }
                break;

            case PINCMD_SET_OUT_VALUE:
                if(meta.mux[mi].pin != last_pin) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    mux_i = xpt_gpio_init_raw(meta.mux[mi].pin);
                    if (mux_i == NULL) return XPT_ERROR_INVALID_HANDLE;
                    last_pin = meta.mux[mi].pin;
                }

                ret = xpt_gpio_dir(mux_i, XPT_GPIO_OUT);

                if(ret == XPT_SUCCESS)
                    ret = xpt_gpio_write(mux_i, meta.mux[mi].value);

                if(ret != XPT_SUCCESS) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    return XPT_ERROR_INVALID_RESOURCE;
                }
                break;

            case PINCMD_SET_MODE:
                if(meta.mux[mi].pin != last_pin) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    mux_i = xpt_gpio_init_raw(meta.mux[mi].pin);
                    if (mux_i == NULL) return XPT_ERROR_INVALID_HANDLE;
                    last_pin = meta.mux[mi].pin;
                }

                ret = xpt_gpio_mode(mux_i, meta.mux[mi].value);

                if(ret != XPT_SUCCESS) {
                    if (mux_i != NULL) {
                        xpt_gpio_owner(mux_i, 0);
                        xpt_gpio_close(mux_i);
                    }
                    return XPT_ERROR_INVALID_RESOURCE;
                }
                break;

            case PINCMD_SKIP:
                break;

            default:
                syslog(LOG_NOTICE, "xpt_setup_mux_mapped: wrong command %d on pin %d with value %d", meta.mux[mi].pincmd, meta.mux[mi].pin, meta.mux[mi].value);
                break;
        }
    }

    if (mux_i != NULL) {
        xpt_gpio_owner(mux_i, 0);
        xpt_gpio_close(mux_i);
    }

    return XPT_SUCCESS;
}
#else
xpt_result_t
xpt_setup_mux_mapped(xpt_pin_t meta)
{
    return XPT_ERROR_FEATURE_NOT_IMPLEMENTED;
}
#endif

void
xpt_result_print(xpt_result_t result)
{
    switch (result) {
        case XPT_SUCCESS:
            fprintf(stdout, "XPT: SUCCESS\n");
            break;
        case XPT_ERROR_FEATURE_NOT_IMPLEMENTED:
            fprintf(stdout, "XPT: Feature not implemented.\n");
            break;
        case XPT_ERROR_FEATURE_NOT_SUPPORTED:
            fprintf(stdout, "XPT: Feature not supported by Hardware.\n");
            break;
        case XPT_ERROR_INVALID_VERBOSITY_LEVEL:
            fprintf(stdout, "XPT: Invalid verbosity level.\n");
            break;
        case XPT_ERROR_INVALID_PARAMETER:
            fprintf(stdout, "XPT: Invalid parameter.\n");
            break;
        case XPT_ERROR_INVALID_HANDLE:
            fprintf(stdout, "XPT: Invalid Handle.\n");
            break;
        case XPT_ERROR_NO_RESOURCES:
            fprintf(stdout, "XPT: No resources.\n");
            break;
        case XPT_ERROR_INVALID_RESOURCE:
            fprintf(stdout, "XPT: Invalid resource.\n");
            break;
        case XPT_ERROR_INVALID_QUEUE_TYPE:
            fprintf(stdout, "XPT: Invalid Queue Type.\n");
            break;
        case XPT_ERROR_NO_DATA_AVAILABLE:
            fprintf(stdout, "XPT: No Data available.\n");
            break;
        case XPT_ERROR_INVALID_PLATFORM:
            fprintf(stdout, "XPT: Platform not recognised.\n");
            break;
        case XPT_ERROR_PLATFORM_NOT_INITIALISED:
            fprintf(stdout, "XPT: Platform not initialised.\n");
            break;
        case XPT_ERROR_UART_OW_SHORTED:
            fprintf(stdout, "XPT: UART OW: Bus short detected.\n");
            break;
        case XPT_ERROR_UART_OW_NO_DEVICES:
            fprintf(stdout, "XPT: UART OW: No devices detected on bus.\n");
            break;
        case XPT_ERROR_UART_OW_DATA_ERROR:
            fprintf(stdout, "XPT: UART OW: Data or Bus error detected.\n");
            break;
        case XPT_ERROR_UNSPECIFIED:
            fprintf(stdout, "XPT: Unspecified Error.\n");
            break;
        default:
            fprintf(stdout, "XPT: Unrecognised error.\n");
            break;
    }
}


xpt_boolean_t
xpt_has_sub_platform()
{
    return (plat != NULL) && (plat->sub_platform != NULL);
}

xpt_boolean_t
xpt_pin_mode_test(int pin, xpt_pinmodes_t mode)
{
    if (plat == NULL)
        return 0;

    xpt_board_t* current_plat = plat;
    if (xpt_is_sub_platform_id(pin)) {
        current_plat = plat->sub_platform;
        if (current_plat == NULL) {
            syslog(LOG_ERR, "xpt_pin_mode_test: Sub platform Not Initialised");
            return 0;
        }
        pin = xpt_get_sub_platform_index(pin);
    }

    if (current_plat == NULL || current_plat->platform_type == XPT_UNKNOWN_PLATFORM || current_plat->platform_type == XPT_NULL_PLATFORM) {
        return 0;
    }
    if (pin > (current_plat->phy_pin_count - 1) || pin < 0)
        return 0;

    switch (mode) {
        case XPT_PIN_VALID:
            if (current_plat->pins[pin].capabilities.valid == 1)
                return 1;
            break;
        case XPT_PIN_GPIO:
            if (current_plat->pins[pin].capabilities.gpio == 1)
                return 1;
            break;
        case XPT_PIN_PWM:
            if (current_plat->pins[pin].capabilities.pwm == 1)
                return 1;
            break;
        case XPT_PIN_FAST_GPIO:
            if (current_plat->pins[pin].capabilities.fast_gpio == 1)
                return 1;
            break;
        case XPT_PIN_SPI:
            if (current_plat->pins[pin].capabilities.spi == 1)
                return 1;
            break;
        case XPT_PIN_I2C:
            if (current_plat->pins[pin].capabilities.i2c == 1)
                return 1;
            break;
        case XPT_PIN_AIO:
            if (current_plat->pins[pin].capabilities.aio == 1)
                return 1;
            break;
        case XPT_PIN_UART:
            if (current_plat->pins[pin].capabilities.uart == 1)
                return 1;
            break;
        default:
            syslog(LOG_NOTICE, "requested pinmode invalid");
            break;
    }
    return 0;
}

xpt_platform_t
xpt_get_platform_type()
{
    if (plat == NULL)
        return XPT_UNKNOWN_PLATFORM;
    return plat->platform_type;
}

int
xpt_get_platform_combined_type()
{
    int type = xpt_get_platform_type();
    int sub_type = xpt_has_sub_platform() ? plat->sub_platform->platform_type : XPT_UNKNOWN_PLATFORM;
    return type | (sub_type << 8);
}

unsigned int
xpt_adc_raw_bits()
{
    if (plat == NULL)
        return 0;

    if (plat->aio_count == 0)
        return 0;

    return plat->adc_raw;
}

unsigned int
xpt_get_platform_adc_raw_bits(uint8_t platform_offset)
{
    if (platform_offset == XPT_MAIN_PLATFORM_OFFSET)
        return xpt_adc_raw_bits();
    else {
        if (!xpt_has_sub_platform())
            return 0;

        if (plat->sub_platform->aio_count == 0)
            return 0;

        return plat->sub_platform->adc_raw;
    }
}


unsigned int
xpt_adc_supported_bits()
{
    if (plat == NULL)
        return 0;

    if (plat->aio_count == 0)
        return 0;

    return plat->adc_supported;
}

unsigned int
xpt_get_platform_adc_supported_bits(int platform_offset)
{
    if (platform_offset == XPT_MAIN_PLATFORM_OFFSET)
        return xpt_adc_supported_bits();
    else {
        if (!xpt_has_sub_platform())
            return 0;

        if (plat->sub_platform->aio_count == 0)
            return 0;

        return plat->sub_platform->adc_supported;
    }
}

const char*
xpt_get_platform_name()
{
    return platform_name;
}

const char*
xpt_get_platform_version(int platform_offset)
{
    if (plat == NULL) {
        return NULL;
    }
    if (platform_offset == XPT_MAIN_PLATFORM_OFFSET) {
        return plat->platform_version;
    } else {
        return plat->sub_platform->platform_version;
    }
}

int
xpt_get_uart_count(void)
{
    if (plat == NULL) {
        return -1;
    }
    return plat->uart_dev_count;
}

int
xpt_get_spi_count(void)
{
    if (plat == NULL) {
        return -1;
    }
    return plat->spi_bus_count;
}

int
xpt_get_pwm_count(void)
{
    if (plat == NULL) {
        return -1;
    }
    return plat->pwm_dev_count;
}

int
xpt_get_gpio_count(void)
{
    if (plat == NULL) {
        return -1;
    }
    return plat->gpio_count;
}

int
xpt_get_aio_count(void)
{
    if (plat == NULL) {
        return -1;
    }
    return plat->aio_count;
}

int
xpt_get_i2c_bus_count()
{
    if (plat == NULL) {
        return -1;
    }
    return plat->i2c_bus_count;
}

int
xpt_get_i2c_bus_id(int i2c_bus)
{
    if (plat == NULL) {
        return -1;
    }

    if (i2c_bus >= plat->i2c_bus_count) {
        return -1;
    }

    return plat->i2c_bus[i2c_bus].bus_id;
}

unsigned int
xpt_get_pin_count()
{
    if (plat == NULL) {
        return 0;
    }
    return plat->phy_pin_count;
}

unsigned int
xpt_get_platform_pin_count(uint8_t platform_offset)
{
    if (platform_offset == XPT_MAIN_PLATFORM_OFFSET)
        return xpt_get_pin_count();
    else {
        if (xpt_has_sub_platform())
           return plat->sub_platform->phy_pin_count;
        else
           return 0;
    }
}


char*
xpt_get_pin_name(int pin)
{
    if (plat == NULL) {
        return 0;
    }

    xpt_board_t* current_plat = plat;
    if (xpt_is_sub_platform_id(pin)) {
        current_plat = plat->sub_platform;
        if (current_plat == NULL) {
            syslog(LOG_ERR, "xpt_get_pin_name: Sub platform Not Initialised");
            return 0;
        }
        pin = xpt_get_sub_platform_index(pin);
    }

    if (pin > (current_plat->phy_pin_count - 1) || pin < 0) {
        return NULL;
    }
    return (char*) current_plat->pins[pin].name;
}

int
xpt_gpio_lookup(const char* pin_name)
{
    int i;

    if (plat == NULL) {
        return -1;
    }

    if (pin_name == NULL || strlen(pin_name) == 0) {
        return -1;
    }

    for (i = 0; i < plat->gpio_count; i++) {
         if (plat->pins[i].name != NULL &&
             strncmp(pin_name, plat->pins[i].name, strlen(plat->pins[i].name) + 1) == 0) {
             return plat->pins[i].gpio.pinmap;
         }
    }
    return -1;
}

int
xpt_i2c_lookup(const char* i2c_name)
{
    int i;

    if (plat == NULL) {
        return -1;
    }

    if (i2c_name == NULL || strlen(i2c_name) == 0) {
        return -1;
    }

    for (i = 0; i < plat->i2c_bus_count; i++) {
         if (plat->i2c_bus[i].name != NULL &&
             strncmp(i2c_name, plat->i2c_bus[i].name, strlen(plat->i2c_bus[i].name) + 1) == 0) {
             return plat->i2c_bus[i].bus_id;
         }
    }
    return -1;
}

int
xpt_spi_lookup(const char* spi_name)
{
    int i;

    if (plat == NULL) {
        return -1;
    }

    if (spi_name == NULL || strlen(spi_name) == 0) {
        return -1;
    }

    for (i = 0; i < plat->spi_bus_count; i++) {
         if (plat->spi_bus[i].name != NULL &&
             strncmp(spi_name, plat->spi_bus[i].name, strlen(plat->spi_bus[i].name) + 1) == 0) {
             return plat->spi_bus[i].bus_id;
         }
    }
    return -1;
}

int
xpt_pwm_lookup(const char* pwm_name)
{
    int i;

    if (plat == NULL) {
        return -1;
    }

    if (pwm_name == NULL || strlen(pwm_name) == 0) {
        return -1;
    }

    for (i = 0; i < plat->pwm_dev_count; i++) {
         if (plat->pwm_dev[i].name != NULL &&
             strncmp(pwm_name, plat->pwm_dev[i].name, strlen(plat->pwm_dev[i].name) + 1) == 0) {
             return plat->pwm_dev[i].index;
         }
    }
    return -1;
}

int
xpt_uart_lookup(const char* uart_name)
{
    int i;

    if (plat == NULL) {
        return -1;
    }

    if (uart_name == NULL || strlen(uart_name) == 0) {
        return -1;
    }

    for (i = 0; i < plat->uart_dev_count; i++) {
         if (plat->uart_dev[i].name != NULL &&
             strncmp(uart_name, plat->uart_dev[i].name, strlen(plat->uart_dev[i].name) + 1) == 0) {
             return plat->uart_dev[i].index;
         }
    }
    return -1;
}

int
xpt_get_default_i2c_bus(uint8_t platform_offset)
{
    if (plat == NULL)
        return -1;
    if (platform_offset == XPT_MAIN_PLATFORM_OFFSET) {
        return plat->def_i2c_bus;
    } else {
        if (xpt_has_sub_platform())
           return plat->sub_platform->def_i2c_bus;
        else
           return -1;
    }
}

#if !defined(PERIPHERALMAN)

xpt_boolean_t
xpt_file_exist(const char* filename)
{
    glob_t results;
    results.gl_pathc = 0;
    glob(filename, 0, NULL, &results);
    int file_found = results.gl_pathc == 1;
    globfree(&results);
    return file_found;
}

xpt_boolean_t
xpt_file_contains(const char* filename, const char* content)
{
    xpt_boolean_t found = 0;
    if ((filename == NULL) || (content == NULL)) {
        return 0;
    }

    char* file = xpt_file_unglob(filename);
    if (file != NULL) {
        size_t len = 0;
        char* line = NULL;
        FILE* fh = fopen(file, "r");
        if (fh == NULL) {
            free(file);
            return 0;
        }
        while ((getline(&line, &len, fh) != -1) && (found == 0)) {
            if (strstr(line, content)) {
                found = 1;
                break;
            }
        }
        fclose(fh);
        free(file);
        free(line);
    }
    return found;
}

xpt_boolean_t
xpt_file_contains_both(const char* filename, const char* content, const char* content2)
{
    xpt_boolean_t found = 0;
    if ((filename == NULL) || (content == NULL)) {
        return 0;
    }

    char* file = xpt_file_unglob(filename);
    if (file != NULL) {
        size_t len = 0;
        char* line = NULL;
        FILE* fh = fopen(file, "r");
        if (fh == NULL) {
            free(file);
            return 0;
        }
        while ((getline(&line, &len, fh) != -1) && (found == 0)) {
            if (strstr(line, content) && strstr(line, content2)) {
                found = 1;
                break;
            }
        }
        fclose(fh);
        free(file);
        free(line);
    }
    return found;
}

char*
xpt_file_unglob(const char* filename)
{
    glob_t results;
    char* res = NULL;
    results.gl_pathc = 0;
    glob(filename, 0, NULL, &results);
    if (results.gl_pathc == 1)
        res = strdup(results.gl_pathv[0]);
    globfree(&results);
    return res;
}

xpt_boolean_t
xpt_link_targets(const char* filename, const char* targetname)
{
    int size = 100;
    int nchars = 0;
    char* buffer = NULL;
    while (nchars == 0) {
        buffer = (char*) realloc(buffer, size);
        if (buffer == NULL)
            return 0;
        nchars = readlink(filename, buffer, size);
        if (nchars < 0) {
            free(buffer);
            return 0;
        } else {
            buffer[nchars] = '\0';
        }
        if (nchars >= size) {
            size *= 2;
            nchars = 0;
        }
    }
    if (strstr(buffer, targetname)) {
        free(buffer);
        return 1;
    } else {
        free(buffer);
        return 0;
    }
}

static int
xpt_count_i2c_files(const char* path, const struct stat* sb, int flag, struct FTW* ftwb)
{
    switch (sb->st_mode & S_IFMT) {
        case S_IFLNK:
            num_i2c_devices++;
            break;
    }
    return 0;
}

int
xpt_find_i2c_bus_pci(const char* pci_device, const char *pci_id, const char* adapter_name)
{
    /**
     * For example we'd get something like:
     * pci0000:00/0000:00:16.3/i2c_desiignware.3
     */
    char path[1024];
    snprintf(path, 1024-1, "/sys/devices/pci%s/%s/%s/", pci_device, pci_id, adapter_name);
    if (xpt_file_exist(path)) {
        struct dirent **namelist;
        int n;
        n = scandir(path, &namelist, NULL, alphasort);
        if (n < 0) {
            syslog(LOG_ERR, "Failed to get information on i2c");
            return -1;
        }
        else {
            while (n--) {
                char* dup = strdup(namelist[n]->d_name);
                char* orig_dup = dup;
                if (dup == NULL) {
                    syslog(LOG_ERR, "Ran out of memory!");
                    break;
                }
                const char delim = '-';
                char* token;
                token = strsep(&dup, &delim);
                if (token != NULL) {
                    if (strncmp("i2c", token, 3) == 0) {
                        token = strsep(&dup, &delim);
                        if (token != NULL) {
                            int ret = -1;
                            if (xpt_atoi(token, &ret) == XPT_SUCCESS) {
                                free(orig_dup);
                                free(namelist[n]);
                                free(namelist);
                                syslog(LOG_NOTICE, "Adding i2c bus found on i2c-%d on adapter %s", ret, adapter_name);
                                return ret;
                            }
                            free(orig_dup);
                            free(namelist[n]);
                            free(namelist);
                            return -1;
                        }
                    }
                }
                free(orig_dup);
                free(namelist[n]);
            }
            free(namelist);
        }
    }
    return -1;
}

int
xpt_find_i2c_bus(const char* devname, int startfrom)
{
    char path[64];
    int fd;
    // because feeding xpt_find_i2c_bus result back into the function is
    // useful treat -1 as 0
    int i = (startfrom < 0) ? 0 : startfrom;
    int ret = -1;

    // find how many i2c buses we have if we haven't already
    if (num_i2c_devices == 0) {
        if (nftw("/sys/class/i2c-dev/", &xpt_count_i2c_files, 20, FTW_PHYS) == -1) {
            return -1;
        }
    }

    // i2c devices are numbered numerically so 0 must exist otherwise there is
    // no i2c-dev loaded
    if (xpt_file_exist("/sys/class/i2c-dev/i2c-0")) {
        for (;i < num_i2c_devices; i++) {
            off_t size, err;
            snprintf(path, 64, "/sys/class/i2c-dev/i2c-%u/name", i);
            fd = open(path, O_RDONLY);
            if (fd < 0) {
                break;
            }
            size = lseek(fd, 0, SEEK_END);
            if (size < 0) {
                syslog(LOG_WARNING, "xpt: failed to seek i2c filename file");
                close(fd);
                break;
            }
            err = lseek(fd, 0, SEEK_SET);
            if (err < 0) {
                syslog(LOG_WARNING, "xpt: failed to seek i2c filename file");
                close(fd);
                break;
            }
            char* value = malloc(size);
            if (value == NULL) {
                syslog(LOG_ERR, "xpt: failed to allocate memory for i2c file");
                close(fd);
                break;
            }
            ssize_t r = read(fd, value, size);
            if (r > 0) {
                if (strcasestr(value, devname) != NULL) {
                    free(value);
                    close(fd);
                    return i;
                }
            } else {
                syslog(LOG_ERR, "xpt: sysfs i2cdev failed");
            }
            free(value);
            close(fd);
        }
    } else {
        syslog(LOG_WARNING, "xpt: no i2c-dev detected, load i2c-dev");
    }

    return ret;
}

#endif

xpt_boolean_t
xpt_is_sub_platform_id(int pin_or_bus)
{
    return (pin_or_bus & XPT_SUB_PLATFORM_MASK) != 0;
}

int
xpt_get_sub_platform_id(int pin_or_bus)
{
    return pin_or_bus | XPT_SUB_PLATFORM_MASK;
}

int
xpt_get_sub_platform_index(int pin_or_bus)
{
    return pin_or_bus & (~XPT_SUB_PLATFORM_MASK);
}

int
xpt_get_iio_device_count()
{
#if defined(PERIPHERALMAN)
    return -1;
#else
    return plat_iio->iio_device_count;
#endif
}

xpt_result_t
xpt_add_subplatform(xpt_platform_t subplatformtype, const char* dev)
{
#if defined(FIRMATA)
    if (subplatformtype == XPT_GENERIC_FIRMATA) {
        if (plat->sub_platform != NULL) {
            if (plat->sub_platform->platform_type == subplatformtype) {
                syslog(LOG_NOTICE, "xpt: Firmata subplatform already present");
                return XPT_SUCCESS;
            }
            syslog(LOG_NOTICE, "xpt: We don't support multiple firmata subplatforms!");
            return XPT_ERROR_FEATURE_NOT_SUPPORTED;
        }
        if (xpt_firmata_platform(plat, dev) == XPT_GENERIC_FIRMATA) {
            syslog(LOG_NOTICE, "xpt: Added firmata subplatform");
            return XPT_SUCCESS;
        }
    }
#else
    if (subplatformtype == XPT_GENERIC_FIRMATA) {
        syslog(LOG_NOTICE, "xpt: Cannot add Firmata platform as support not compiled in");
    }
#endif

    if (subplatformtype == XPT_GROVEPI) {
        if (plat == NULL || plat->platform_type == XPT_UNKNOWN_PLATFORM || plat->i2c_bus_count == 0) {
            syslog(LOG_NOTICE, "xpt: The GrovePi shield is not supported on this platform!");
            return XPT_ERROR_FEATURE_NOT_SUPPORTED;
        }
        if (plat->sub_platform != NULL) {
            syslog(LOG_NOTICE, "xpt: A subplatform was already added!");
            return XPT_ERROR_FEATURE_NOT_SUPPORTED;
        }
        int i2c_bus;
        if(xpt_atoi(strdup(dev), &i2c_bus) != XPT_SUCCESS && i2c_bus < plat->i2c_bus_count) {
            syslog(LOG_NOTICE, "xpt: Cannot add GrovePi subplatform, invalid i2c bus specified");
            return XPT_ERROR_INVALID_PARAMETER;
        }
        if (xpt_grovepi_platform(plat, i2c_bus) == XPT_GROVEPI) {
            syslog(LOG_NOTICE, "xpt: Added GrovePi subplatform");
            return XPT_SUCCESS;
        }
    }

    return XPT_ERROR_INVALID_PARAMETER;
}

xpt_result_t
xpt_remove_subplatform(xpt_platform_t subplatformtype)
{
    if (subplatformtype != XPT_FTDI_FT4222) {
        if (plat == NULL || plat->sub_platform == NULL) {
            return XPT_ERROR_INVALID_PARAMETER;
        }
        free(plat->sub_platform->adv_func);
        free(plat->sub_platform->pins);
        free(plat->sub_platform);
        return XPT_SUCCESS;
    }
    return XPT_ERROR_INVALID_PARAMETER;
}

#if defined(IXPT)
xpt_result_t
xpt_add_from_lockfile(const char* ixpt_lock_file)
{
    xpt_result_t ret = XPT_SUCCESS;
    char* buffer = NULL;
    struct stat st;
    int i = 0;
    uint32_t subplat_num = 0;
    int flock = open(ixpt_lock_file, O_RDONLY);
    if (flock == -1) {
        syslog(LOG_ERR, "ixpt: Failed to open lock file");
        return XPT_ERROR_INVALID_RESOURCE;
    }
    if (fstat(flock, &st) != 0 || (!S_ISREG(st.st_mode))) {
        close(flock);
        return XPT_ERROR_INVALID_RESOURCE;
    }
    buffer = mmap(0, st.st_size, PROT_READ, MAP_SHARED, flock, 0);
    close(flock);
    if (buffer == MAP_FAILED) {
        syslog(LOG_ERR, "ixpt: lockfile read error");
        return XPT_ERROR_INVALID_RESOURCE;
    }
    json_object* jobj_lock = json_tokener_parse(buffer);

    struct json_object* ioarray;
    if (json_object_object_get_ex(jobj_lock, "Platform", &ioarray) == true &&
        json_object_is_type(ioarray, json_type_array)) {
        subplat_num = json_object_array_length(ioarray);
        int id = -1;
        const char* uartdev = NULL;
        for (i = 0; i < subplat_num; i++) {
            struct json_object *ioobj = json_object_array_get_idx(ioarray, i);
            json_object_object_foreach(ioobj, key, val) {
                if (strncmp(key, "id", strlen("id") + 1) == 0) {
                    if (xpt_atoi(json_object_get_string(val), &id) != XPT_SUCCESS) {
                        id = -1;
                    }
                } else if (strncmp(key, "uart", strlen("uart") + 1) == 0) {
                    uartdev = json_object_get_string(val);
                }
            }
            if (id != -1 && id != XPT_NULL_PLATFORM && id != XPT_UNKNOWN_PLATFORM && uartdev != NULL) {
                if (xpt_add_subplatform(id, uartdev) == XPT_SUCCESS) {
                    syslog(LOG_NOTICE, "ixpt: automatically added subplatform %d, %s", id, uartdev);
                } else {
                    syslog(LOG_ERR, "ixpt: Failed to add subplatform (%d on %s) from lockfile", id, uartdev);
                }
                id = -1;
                uartdev = NULL;
            }
        }
        if (json_object_object_get_ex(jobj_lock, "IO", &ioarray) == true &&
            json_object_is_type(ioarray, json_type_array)) {
	    /* assume we have declared IO so we are preinitialised and wipe the
	     * advance func array
             */
            memset(plat->adv_func, 0, sizeof(xpt_adv_func_t));
        }
    }
    else {
        ret = XPT_ERROR_INVALID_RESOURCE;
    }
    json_object_put(jobj_lock);
    munmap(buffer, st.st_size);
    return ret;
}
#endif

void
xpt_to_upper(char* s)
{
    char* t = s;
    for (; *t; ++t) {
        *t = toupper(*t);
    }
}

xpt_result_t
xpt_atoi(char* intStr, int* value)
{
    char* end;
    // here 10 determines the number base in which strol is to work
    long val = strtol(intStr, &end, 10);
    if (*end != '\0' || errno == ERANGE || end == intStr || val > INT_MAX || val < INT_MIN) {
        *value = 0;
        return XPT_ERROR_UNSPECIFIED;
    }
    *value = (int) val;
    return XPT_SUCCESS;
}

xpt_result_t
xpt_init_io_helper(char** str, int* value, const char* delim)
{
    // This function is a result of a repeated pattern within xpt_init_io
    // when determining if a value can be derived from a string
    char* token;
    token = strsep(str, delim);
    // check to see if empty string returned
    if (token == NULL) {
        *value = 0;
        return XPT_ERROR_NO_DATA_AVAILABLE;
    }
    return xpt_atoi(token, value);
}

void*
xpt_init_io(const char* desc)
{
    const char* delim = "-";
    int length = 0, raw = 0;
    int pin = 0, id = 0;
    // 256 denotes the maximum size of our buffer
    // 8 denotes the maximum size of our type rounded to the nearest power of 2
    // max size is 4 + 1 for the \0 = 5 rounded to 8
    char buffer[256] = { 0 }, type[8] = { 0 };
    char *token = 0, *str = 0;
    if (desc == NULL) {
        return NULL;
    }
    length = strlen(desc);
    // Check to see the length is less than or equal to 255 which means
    // byte 256 is supposed to be \0
    if (length > 255 || length == 0) {
        return NULL;
    }
    strncpy(buffer, desc, length);

    str = buffer;
    token = strsep(&str, delim);
    length = strlen(token);
    // Check to see they haven't given us a type whose length is greater than the
    // largest type we know about
    if (length > 4) {
        syslog(LOG_ERR, "xpt_init_io: An invalid IO type was provided");
        return NULL;
    }
    strncpy(type, token, length);
    xpt_to_upper(type);
    token = strsep(&str, delim);
    // Check that they've given us more information than just the type
    if (token == NULL) {
        syslog(LOG_ERR, "xpt_init_io: Missing information after type");
        return NULL;
    }
    // If we cannot convert the pin to a number maybe it says raw?
    if (xpt_atoi(token, &pin) != XPT_SUCCESS) {
        xpt_to_upper(token);
        if (strncmp(token, "RAW", strlen("RAW") + 1)) {
            syslog(LOG_ERR, "xpt_init_io: Description does not adhere to a known format");
            return NULL;
        }
        raw = 1;
    }
    if (!raw && str != NULL) {
        syslog(LOG_ERR, "xpt_init_io: More information than required was provided");
        return NULL;
    }

    if (strncmp(type, GPIO_KEY, strlen(GPIO_KEY) + 1) == 0) {
        if (raw) {
            if (xpt_init_io_helper(&str, &pin, delim) == XPT_SUCCESS) {
                return (void*) xpt_gpio_init_raw(pin);
            }
            syslog(LOG_ERR, "xpt_init_io: Invalid Raw description for GPIO");
            return NULL;
        }
        return (void*) xpt_gpio_init(pin);
    } else if (strncmp(type, I2C_KEY, strlen(I2C_KEY) + 1) == 0) {
        if (raw) {
            if (xpt_init_io_helper(&str, &pin, delim) == XPT_SUCCESS) {
                return (void*) xpt_i2c_init_raw(pin);
            }
            syslog(LOG_ERR, "xpt_init_io: Invalid Raw description for I2C");
            return NULL;
        }
        return (void*) xpt_i2c_init(pin);
    } else if (strncmp(type, AIO_KEY, strlen(AIO_KEY) + 1) == 0) {
        if (raw) {
            syslog(LOG_ERR, "xpt_init_io: Aio doesn't have a RAW mode");
            return NULL;
        }
        return (void*) xpt_aio_init(pin);
    } else if (strncmp(type, PWM_KEY, strlen(PWM_KEY) + 1) == 0) {
        if (raw) {
            if (xpt_init_io_helper(&str, &id, delim) != XPT_SUCCESS) {
                syslog(LOG_ERR, "xpt_init_io: Pwm, unable to convert the chip id string into a useable Int");
                return NULL;
            }
            if (xpt_init_io_helper(&str, &pin, delim) != XPT_SUCCESS) {
                syslog(LOG_ERR, "xpt_init_io: Pwm, unable to convert the pin string into a useable Int");
                return NULL;
            }
            return (void*) xpt_pwm_init_raw(id, pin);
        }
        return (void*) xpt_pwm_init(pin);
    } else if (strncmp(type, SPI_KEY, strlen(SPI_KEY) + 1) == 0) {
        if (raw) {
            if (xpt_init_io_helper(&str, &id, delim) != XPT_SUCCESS) {
                syslog(LOG_ERR, "xpt_init_io: Spi, unable to convert the bus string into a useable Int");
                return NULL;
            }
            if (xpt_init_io_helper(&str, &pin, delim) != XPT_SUCCESS) {
                syslog(LOG_ERR, "xpt_init_io: Spi, unable to convert the cs string into a useable Int");
                return NULL;
            }
            return (void*) xpt_spi_init_raw(id, pin);
        }
        return (void*) xpt_spi_init(pin);
    } else if (strncmp(type, UART_KEY, strlen(UART_KEY) + 1) == 0) {
        if (raw) {
            return (void*) xpt_uart_init_raw(str);
        }
        return (void*) xpt_uart_init(pin);
    }
    syslog(LOG_ERR, "xpt_init_io: Invalid IO type given.");
    return NULL;
}


#ifndef JSONPLAT
xpt_result_t
xpt_init_json_platform(const char* desc)
{
    return XPT_ERROR_FEATURE_NOT_SUPPORTED;
}
#endif
