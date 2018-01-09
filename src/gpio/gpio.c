#include "gpio.h"
#include "xpt_internal.h"

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#define SYSFS_CLASS_GPIO "/sys/class/gpio"
#define MAX_SIZE 64
#define POLL_TIMEOUT

static xpt_result_t
xpt_gpio_get_valfp(xpt_gpio_context dev)
{
    char bu[MAX_SIZE];
    snprintf(bu, MAX_SIZE, SYSFS_CLASS_GPIO "/gpio%d/value", dev->pin);
    dev->value_fp = open(bu, O_RDWR);
    if (dev->value_fp == -1) {
        syslog(LOG_ERR, "gpio%i: Failed to open 'value': %s", dev->pin, strerror(errno));
        return XPT_ERROR_INVALID_RESOURCE;
    }

    return XPT_SUCCESS;
}

static xpt_gpio_context
xpt_gpio_init_internal(xpt_adv_func_t* func_table, int pin)
{
    if (pin < 0)
        return NULL;

    xpt_result_t status = XPT_SUCCESS;
    char bu[MAX_SIZE];
    int length;

    xpt_gpio_context dev = (xpt_gpio_context) calloc(1, sizeof(struct _gpio));
    if (dev == NULL) {
        syslog(LOG_CRIT, "gpio%i: Failed to allocate memory for context", pin);
        return NULL;
    }

    dev->advance_func = func_table;
    dev->pin = pin;

    if (IS_FUNC_DEFINED(dev, gpio_init_internal_replace)) {
        status = dev->advance_func->gpio_init_internal_replace(dev, pin);
        if (status == XPT_SUCCESS)
            return dev;
        else
            goto init_internal_cleanup;
    }

    if (IS_FUNC_DEFINED(dev, gpio_init_pre)) {
        status = dev->advance_func->gpio_init_pre(pin);
        if (status != XPT_SUCCESS)
            goto init_internal_cleanup;
    }

    dev->value_fp = -1;
    dev->isr_value_fp = -1;
#ifndef HAVE_PTHREAD_CANCEL
    dev->isr_control_pipe[0] = dev->isr_control_pipe[1] = -1;
#endif
    dev->isr_thread_terminating = 0;
    dev->phy_pin = -1;

    // then check to make sure the pin is exported.
    char directory[MAX_SIZE];
    snprintf(directory, MAX_SIZE, SYSFS_CLASS_GPIO "/gpio%d/", dev->pin);
    struct stat dir;
    if (stat(directory, &dir) == 0 && S_ISDIR(dir.st_mode)) {
        dev->owner = 0; // Not Owner
    } else {
        int export = open(SYSFS_CLASS_GPIO "/export", O_WRONLY);
        if (export == -1) {
            syslog(LOG_ERR, "gpio%i: init: Failed to open 'export' for writing: %s", pin, strerror(errno));
            status = XPT_ERROR_INVALID_RESOURCE;
            goto init_internal_cleanup;
        }
        length = snprintf(bu, sizeof(bu), "%d", dev->pin);
        if (write(export, bu, length * sizeof(char)) == -1) {
            syslog(LOG_ERR, "gpio%i: init: Failed to write to 'export': %s", pin, strerror(errno));
            close(export);
            status = XPT_ERROR_INVALID_RESOURCE;
            goto init_internal_cleanup;
        }
        dev->owner = 1;
        close(export);
    }

init_internal_cleanup:
    if (status != XPT_SUCCESS) {
        if (dev != NULL)
            free(dev);
        return NULL;
    }
    return dev;
}

xpt_gpio_context
xpt_gpio_init(int pin)
{
    xpt_board_t* board = plat;
    if (board == NULL) {
        syslog(LOG_ERR, "gpio%i: init: platform not initialised", pin);
        return NULL;
    }

    if (xpt_is_sub_platform_id(pin)) {
        syslog(LOG_NOTICE, "gpio%i: init: Using sub platform", pin);
        board = board->sub_platform;
        if (board == NULL) {
            syslog(LOG_ERR, "gpio%i: init: Sub platform not initialised", pin);
            return NULL;
        }
        pin = xpt_get_sub_platform_index(pin);
    }

    if (pin < 0 || pin >= board->phy_pin_count) {
        syslog(LOG_ERR, "gpio: init: pin %i beyond platform pin count (%i)", pin, board->phy_pin_count);
        return NULL;
    }
    if (board->pins[pin].capabilities.gpio != 1) {
        syslog(LOG_ERR, "gpio: init: pin %i not capable of gpio", pin);
        return NULL;
    }
    if (board->pins[pin].gpio.mux_total > 0) {
        if (xpt_setup_mux_mapped(board->pins[pin].gpio) != XPT_SUCCESS) {
            syslog(LOG_ERR, "gpio%i: init: unable to setup muxes", pin);
            return NULL;
        }
    }

    xpt_gpio_context r = xpt_gpio_init_internal(board->adv_func, board->pins[pin].gpio.pinmap);
    if (r == NULL) {
        return NULL;
    }
    if (r->phy_pin == -1)
        r->phy_pin = pin;

    if (IS_FUNC_DEFINED(r, gpio_init_post)) {
        xpt_result_t ret = r->advance_func->gpio_init_post(r);
        if (ret != XPT_SUCCESS) {
            free(r);
            return NULL;
        }
    }
    return r;
}

xpt_gpio_context
xpt_gpio_init_raw(int pin)
{
    return xpt_gpio_init_internal(plat == NULL ? NULL : plat->adv_func , pin);
}


static xpt_result_t
xpt_gpio_wait_interrupt(int fd
#ifndef HAVE_PTHREAD_CANCEL
        , int control_fd
#endif
        )
{
    unsigned char c;
#ifdef HAVE_PTHREAD_CANCEL
    struct pollfd pfd[1];
#else
    struct pollfd pfd[2];

    if (control_fd < 0) {
        return XPT_ERROR_INVALID_PARAMETER;
    }
#endif

    if (fd < 0) {
        return XPT_ERROR_INVALID_PARAMETER;
    }

    // setup poll on POLLPRI
    pfd[0].fd = fd;
    pfd[0].events = POLLPRI;

    // do an initial read to clear interrupt
    lseek(fd, 0, SEEK_SET);
    read(fd, &c, 1);

#ifdef HAVE_PTHREAD_CANCEL
    // Wait for it forever or until pthread_cancel
    // poll is a cancelable point like sleep()
    poll(pfd, 1, -1);
#else
    // setup poll on the controling fd
    pfd[1].fd = control_fd;
    pfd[1].events = 0; //  POLLHUP, POLLERR, and POLLNVAL

    // Wait for it forever or until control fd is closed
    poll(pfd, 2, -1);
#endif

    // do a final read to clear interrupt
    read(fd, &c, 1);

    return XPT_SUCCESS;
}

static void*
xpt_gpio_interrupt_handler(void* arg)
{
    if (arg == NULL) {
        syslog(LOG_ERR, "gpio: interrupt_handler: context is invalid");
        return NULL;
    }

    xpt_gpio_context dev = (xpt_gpio_context) arg;
    int fp = -1;
    xpt_result_t ret;

    if (IS_FUNC_DEFINED(dev, gpio_interrupt_handler_init_replace)) {
        if (dev->advance_func->gpio_interrupt_handler_init_replace(dev) != XPT_SUCCESS)
            return NULL;
    } else {
        // open gpio value with open(3)
        char bu[MAX_SIZE];
        snprintf(bu, MAX_SIZE, SYSFS_CLASS_GPIO "/gpio%d/value", dev->pin);
        fp = open(bu, O_RDONLY);
        if (fp < 0) {
            syslog(LOG_ERR, "gpio%i: interrupt_handler: failed to open 'value' : %s", dev->pin, strerror(errno));
            return NULL;
        }
    }

#ifndef HAVE_PTHREAD_CANCEL
    if (pipe(dev->isr_control_pipe)) {
        syslog(LOG_ERR, "gpio%i: interrupt_handler: failed to create isr control pipe: %s", dev->pin, strerror(errno));
        close(fp);
        return NULL;
    }
#endif

    dev->isr_value_fp = fp;

    if (lang_func->java_attach_thread != NULL) {
        if (dev->isr == lang_func->java_isr_callback) {
            if (lang_func->java_attach_thread() != XPT_SUCCESS) {
                close(dev->isr_value_fp);
                dev->isr_value_fp = -1;
                return NULL;
            }
        }
    }

    for (;;) {
        if (IS_FUNC_DEFINED(dev, gpio_wait_interrupt_replace)) {
            ret = dev->advance_func->gpio_wait_interrupt_replace(dev);
        } else {
            ret = xpt_gpio_wait_interrupt(dev->isr_value_fp
#ifndef HAVE_PTHREAD_CANCEL
                , dev->isr_control_pipe[0]
#endif
                );
        }
        if (ret == XPT_SUCCESS && !dev->isr_thread_terminating) {
#ifdef HAVE_PTHREAD_CANCEL
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
#endif
            if (lang_func->python_isr != NULL) {
                lang_func->python_isr(dev->isr, dev->isr_args);
            } else {
                dev->isr(dev->isr_args);
            }
#ifdef HAVE_PTHREAD_CANCEL
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
#endif
        } else {
            // we must have got an error code or exit request so die nicely
#ifdef HAVE_PTHREAD_CANCEL
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
#endif
            if (fp != -1) {
                close(dev->isr_value_fp);
                dev->isr_value_fp = -1;
            }

            if (lang_func->java_detach_thread != NULL && lang_func->java_delete_global_ref != NULL) {
                if (dev->isr == lang_func->java_isr_callback) {
                    lang_func->java_delete_global_ref(dev->isr_args);
                    lang_func->java_detach_thread();
                }
            }

            return NULL;
        }
    }
}

xpt_result_t
xpt_gpio_edge_mode(xpt_gpio_context dev, xpt_gpio_edge_t mode)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: edge_mode: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

   if (IS_FUNC_DEFINED(dev, gpio_edge_mode_replace))
        return dev->advance_func->gpio_edge_mode_replace(dev, mode);

    if (dev->value_fp != -1) {
        close(dev->value_fp);
        dev->value_fp = -1;
    }

    char filepath[MAX_SIZE];
    snprintf(filepath, MAX_SIZE, SYSFS_CLASS_GPIO "/gpio%d/edge", dev->pin);

    int edge = open(filepath, O_RDWR);
    if (edge == -1) {
        syslog(LOG_ERR, "gpio%i: edge_mode: Failed to open 'edge' for writing: %s", dev->pin, strerror(errno));
        return XPT_ERROR_INVALID_RESOURCE;
    }

    char bu[MAX_SIZE];
    int length;
    switch (mode) {
        case XPT_GPIO_EDGE_NONE:
            length = snprintf(bu, sizeof(bu), "none");
            break;
        case XPT_GPIO_EDGE_BOTH:
            length = snprintf(bu, sizeof(bu), "both");
            break;
        case XPT_GPIO_EDGE_RISING:
            length = snprintf(bu, sizeof(bu), "rising");
            break;
        case XPT_GPIO_EDGE_FALLING:
            length = snprintf(bu, sizeof(bu), "falling");
            break;
        default:
            close(edge);
            return XPT_ERROR_FEATURE_NOT_IMPLEMENTED;
    }
    if (write(edge, bu, length * sizeof(char)) == -1) {
        syslog(LOG_ERR, "gpio%i: edge_mode: Failed to write to 'edge': %s", dev->pin, strerror(errno));
        close(edge);
        return XPT_ERROR_UNSPECIFIED;
    }

    close(edge);
    return XPT_SUCCESS;
}

xpt_result_t
xpt_gpio_isr(xpt_gpio_context dev, xpt_gpio_edge_t mode, void (*fptr)(void*), void* args)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: isr: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, gpio_isr_replace)) {
        return dev->advance_func->gpio_isr_replace(dev, mode, fptr, args);
    }

    // we only allow one isr per xpt_gpio_context
    if (dev->thread_id != 0) {
        return XPT_ERROR_NO_RESOURCES;
    }

    xpt_result_t ret = xpt_gpio_edge_mode(dev, mode);
    if (ret != XPT_SUCCESS) {
        return ret;
    }

    dev->isr = fptr;

    /* Most UPM sensors use the C API, the Java global ref must be created here. */
    /* The reason for checking the callback function is internal callbacks. */
    if (lang_func->java_create_global_ref != NULL) {
        if (dev->isr == lang_func->java_isr_callback) {
            args = lang_func->java_create_global_ref(args);
        }
    }

    dev->isr_args = args;
    pthread_create(&dev->thread_id, NULL, xpt_gpio_interrupt_handler, (void*) dev);

    return XPT_SUCCESS;
}

xpt_result_t
xpt_gpio_isr_exit(xpt_gpio_context dev)
{
    xpt_result_t ret = XPT_SUCCESS;

    if (dev == NULL) {
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, gpio_isr_exit_replace)) {
        return dev->advance_func->gpio_isr_exit_replace(dev);
    }

    // wasting our time, there is no isr to exit from
    if (dev->thread_id == 0 && dev->isr_value_fp == -1) {
        return ret;
    }
    // mark the beginning of the thread termination process for interested parties
    dev->isr_thread_terminating = 1;

    // stop isr being useful
    ret = xpt_gpio_edge_mode(dev, XPT_GPIO_EDGE_NONE);

    if ((dev->thread_id != 0)) {
#ifdef HAVE_PTHREAD_CANCEL
        if ((pthread_cancel(dev->thread_id) != 0) || (pthread_join(dev->thread_id, NULL) != 0)) {
            ret = XPT_ERROR_INVALID_RESOURCE;
        }
#else
        close(dev->isr_control_pipe[1]);
        if (pthread_join(dev->thread_id, NULL) != 0)
            ret = XPT_ERROR_INVALID_RESOURCE;

        close(dev->isr_control_pipe[0]);
        dev->isr_control_pipe[0] =  dev->isr_control_pipe[1] = -1;
#endif
    }

    // close the filehandle in case it's still open
    if (dev->isr_value_fp != -1) {
        if (close(dev->isr_value_fp) != 0) {
            ret = XPT_ERROR_INVALID_RESOURCE;
        }
    }

    // assume our thread will exit either way we just lost it's handle
    dev->thread_id = 0;
    dev->isr_value_fp = -1;
    dev->isr_thread_terminating = 0;
    return ret;
}

xpt_result_t
xpt_gpio_mode(xpt_gpio_context dev, xpt_gpio_mode_t mode)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: mode: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, gpio_mode_replace))
        return dev->advance_func->gpio_mode_replace(dev, mode);

    if (IS_FUNC_DEFINED(dev, gpio_mode_pre)) {
        xpt_result_t pre_ret = (dev->advance_func->gpio_mode_pre(dev, mode));
        if (pre_ret != XPT_SUCCESS)
            return pre_ret;
    }

    if (dev->value_fp != -1) {
        close(dev->value_fp);
        dev->value_fp = -1;
    }

    char filepath[MAX_SIZE];
    snprintf(filepath, MAX_SIZE, SYSFS_CLASS_GPIO "/gpio%d/drive", dev->pin);

    int drive = open(filepath, O_WRONLY);
    if (drive == -1) {
        syslog(LOG_ERR, "gpio%i: mode: Failed to open 'drive' for writing: %s", dev->pin, strerror(errno));
        return XPT_ERROR_INVALID_RESOURCE;
    }

    char bu[MAX_SIZE];
    int length;
    switch (mode) {
        case XPT_GPIO_STRONG:
            length = snprintf(bu, sizeof(bu), "strong");
            break;
        case XPT_GPIO_PULLUP:
            length = snprintf(bu, sizeof(bu), "pullup");
            break;
        case XPT_GPIO_PULLDOWN:
            length = snprintf(bu, sizeof(bu), "pulldown");
            break;
        case XPT_GPIO_HIZ:
            length = snprintf(bu, sizeof(bu), "hiz");
            break;
        default:
            close(drive);
            return XPT_ERROR_FEATURE_NOT_IMPLEMENTED;
    }
    if (write(drive, bu, length * sizeof(char)) == -1) {
        syslog(LOG_ERR, "gpio%i: mode: Failed to write to 'drive': %s", dev->pin, strerror(errno));
       close(drive);
        return XPT_ERROR_INVALID_RESOURCE;
    }

    close(drive);
    if (IS_FUNC_DEFINED(dev, gpio_mode_post))
        return dev->advance_func->gpio_mode_post(dev, mode);
    return XPT_SUCCESS;
}

xpt_result_t
xpt_gpio_dir(xpt_gpio_context dev, xpt_gpio_dir_t dir)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: dir: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, gpio_dir_replace)) {
        return dev->advance_func->gpio_dir_replace(dev, dir);
    }

    if (IS_FUNC_DEFINED(dev, gpio_dir_pre)) {
        xpt_result_t pre_ret = (dev->advance_func->gpio_dir_pre(dev, dir));
        if (pre_ret != XPT_SUCCESS) {
            return pre_ret;
        }
    }

    if (dev->value_fp != -1) {
        close(dev->value_fp);
        dev->value_fp = -1;
    }
    char filepath[MAX_SIZE];
    snprintf(filepath, MAX_SIZE, SYSFS_CLASS_GPIO "/gpio%d/direction", dev->pin);

    int direction = open(filepath, O_RDWR);

    if (direction == -1) {
        // Direction Failed to Open. If HIGH or LOW was passed will try and set
        // If not fail as usual.
        switch (dir) {
            case XPT_GPIO_OUT_HIGH:
                return xpt_gpio_write(dev, 1);
            case XPT_GPIO_OUT_LOW:
                return xpt_gpio_write(dev, 0);
            default:
                syslog(LOG_ERR, "gpio%i: dir: Failed to open 'direction' for writing: %s", dev->pin, strerror(errno));
                return XPT_ERROR_INVALID_RESOURCE;
       }
    }

    char bu[MAX_SIZE];
    int length;
    switch (dir) {
        case XPT_GPIO_OUT:
            length = snprintf(bu, sizeof(bu), "out");
            break;
        case XPT_GPIO_IN:
            length = snprintf(bu, sizeof(bu), "in");
            break;
        case XPT_GPIO_OUT_HIGH:
            length = snprintf(bu, sizeof(bu), "high");
            break;
        case XPT_GPIO_OUT_LOW:
            length = snprintf(bu, sizeof(bu), "low");
            break;
        default:
            close(direction);
            return XPT_ERROR_FEATURE_NOT_IMPLEMENTED;
    }

    if (write(direction, bu, length * sizeof(char)) == -1) {
        close(direction);
        syslog(LOG_ERR, "gpio%i: dir: Failed to write to 'direction': %s", dev->pin, strerror(errno));
        return XPT_ERROR_UNSPECIFIED;
    }

    close(direction);
    if (IS_FUNC_DEFINED(dev, gpio_dir_post))
        return dev->advance_func->gpio_dir_post(dev, dir);
    return XPT_SUCCESS;
}

xpt_result_t
xpt_gpio_read_dir(xpt_gpio_context dev, xpt_gpio_dir_t *dir)
{
    char value[5];
    char filepath[MAX_SIZE];
    int fd, rc;
    xpt_result_t result = XPT_SUCCESS;

    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: read_dir: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (dir == NULL) {
        syslog(LOG_ERR, "gpio: read_dir: output parameter for dir is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, gpio_read_dir_replace)) {
        return dev->advance_func->gpio_read_dir_replace(dev, dir);
    }

    snprintf(filepath, MAX_SIZE, SYSFS_CLASS_GPIO "/gpio%d/direction", dev->pin);
    fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        syslog(LOG_ERR, "gpio%i: read_dir: Failed to open 'direction' for reading: %s", dev->pin, strerror(errno));
        return XPT_ERROR_INVALID_RESOURCE;
    }

    memset(value, '\0', sizeof(value));
    rc = read(fd, value, sizeof(value));
    close(fd);
    if (rc <= 0) {
        syslog(LOG_ERR, "gpio%i: read_dir: Failed to read 'direction': %s", dev->pin, strerror(errno));
        return XPT_ERROR_INVALID_RESOURCE;
    }

    if (strcmp(value, "out\n") == 0) {
        *dir = XPT_GPIO_OUT;
    } else if (strcmp(value, "in\n") == 0) {
        *dir = XPT_GPIO_IN;
    } else {
        syslog(LOG_ERR, "gpio%i: read_dir: unknown direction: %s", dev->pin, value);
        result = XPT_ERROR_UNSPECIFIED;
    }

    return result;
}

int
xpt_gpio_read(xpt_gpio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: read: context is invalid");
        return -1;
    }

    if (IS_FUNC_DEFINED(dev, gpio_read_replace))
        return dev->advance_func->gpio_read_replace(dev);

    if (dev->mmap_read != NULL)
        return dev->mmap_read(dev);

    if (dev->value_fp == -1) {
        if (xpt_gpio_get_valfp(dev) != XPT_SUCCESS) {
            return -1;
        }
    } else {
        // if value_fp is new this is pointless
        lseek(dev->value_fp, 0, SEEK_SET);
    }
    char bu[2];
    if (read(dev->value_fp, bu, 2 * sizeof(char)) != 2) {
        syslog(LOG_ERR, "gpio%i: read: Failed to read a sensible value from sysfs: %s", dev->pin, strerror(errno));
        return -1;
    }
    lseek(dev->value_fp, 0, SEEK_SET);

    return (int) strtol(bu, NULL, 10);
}

xpt_result_t
xpt_gpio_write(xpt_gpio_context dev, int value)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: write: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (dev->mmap_write != NULL)
        return dev->mmap_write(dev, value);

    if (IS_FUNC_DEFINED(dev, gpio_write_pre)) {
        xpt_result_t pre_ret = (dev->advance_func->gpio_write_pre(dev, value));
        if (pre_ret != XPT_SUCCESS)
            return pre_ret;
    }

    if (IS_FUNC_DEFINED(dev, gpio_write_replace)) {
        return dev->advance_func->gpio_write_replace(dev, value);
    }

    if (dev->value_fp == -1) {
        if (xpt_gpio_get_valfp(dev) != XPT_SUCCESS) {
            return XPT_ERROR_INVALID_RESOURCE;
        }
    }

    if (lseek(dev->value_fp, 0, SEEK_SET) == -1) {
        syslog(LOG_ERR, "gpio%i: write: Failed to lseek 'value': %s", dev->pin, strerror(errno));
          return XPT_ERROR_UNSPECIFIED;
    }

    char bu[MAX_SIZE];
    int length = snprintf(bu, sizeof(bu), "%d", value);
    if (write(dev->value_fp, bu, length * sizeof(char)) == -1) {
        syslog(LOG_ERR, "gpio%i: write: Failed to write to 'value': %s", dev->pin, strerror(errno));
        return XPT_ERROR_UNSPECIFIED;
    }

    if (IS_FUNC_DEFINED(dev, gpio_write_post))
        return dev->advance_func->gpio_write_post(dev, value);
    return XPT_SUCCESS;
}

static xpt_result_t
xpt_gpio_unexport_force(xpt_gpio_context dev)
{
    int unexport = open(SYSFS_CLASS_GPIO "/unexport", O_WRONLY);
    if (unexport == -1) {
        syslog(LOG_ERR, "gpio%i: Failed to open 'unexport' for writing: %s", dev->pin, strerror(errno));
        return XPT_ERROR_INVALID_RESOURCE;
    }

    char bu[MAX_SIZE];
    int length = snprintf(bu, sizeof(bu), "%d", dev->pin);
    if (write(unexport, bu, length * sizeof(char)) == -1) {
        syslog(LOG_ERR, "gpio%i: Failed to write to 'unexport': %s", dev->pin, strerror(errno));
        close(unexport);
        return XPT_ERROR_UNSPECIFIED;
    }

    close(unexport);
    xpt_gpio_isr_exit(dev);
    return XPT_SUCCESS;
}
static xpt_result_t
xpt_gpio_unexport(xpt_gpio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: unexport: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (dev->owner) {
        return xpt_gpio_unexport_force(dev);
    }
    return XPT_ERROR_INVALID_PARAMETER;
}

xpt_result_t
xpt_gpio_close(xpt_gpio_context dev)
{
    xpt_result_t result = XPT_SUCCESS;

    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: close: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, gpio_close_replace)) {
        return dev->advance_func->gpio_close_replace(dev);
    }


    if (IS_FUNC_DEFINED(dev, gpio_close_pre)) {
        result = dev->advance_func->gpio_close_pre(dev);
    }

    if (dev->value_fp != -1) {
        close(dev->value_fp);
    }
    xpt_gpio_unexport(dev);
    free(dev);
    return result;
}

xpt_result_t
xpt_gpio_owner(xpt_gpio_context dev, xpt_boolean_t own)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: owner: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }
    syslog(LOG_DEBUG, "gpio%i: owner: Set owner to %d", dev->pin, (int) own);
    dev->owner = own;
    return XPT_SUCCESS;
}

xpt_result_t
xpt_gpio_use_mmaped(xpt_gpio_context dev, xpt_boolean_t mmap_en)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: use_mmaped: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, gpio_mmap_setup)) {
        return dev->advance_func->gpio_mmap_setup(dev, mmap_en);
    }

    syslog(LOG_ERR, "gpio%i: use_mmaped: mmap not implemented on this platform", dev->pin);
    return XPT_ERROR_FEATURE_NOT_IMPLEMENTED;
}

int
xpt_gpio_get_pin(xpt_gpio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: get_pin: context is invalid");
        return -1;
    }
    return dev->phy_pin;
}

int
xpt_gpio_get_pin_raw(xpt_gpio_context dev)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: get_pin: context is invalid");
        return -1;
    }
    return dev->pin;
}

xpt_result_t
xpt_gpio_input_mode(xpt_gpio_context dev, xpt_gpio_input_mode_t mode)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: in_mode: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    char filepath[MAX_SIZE];
    snprintf(filepath, MAX_SIZE, SYSFS_CLASS_GPIO "/gpio%d/active_low", dev->pin);

    int active_low = open(filepath, O_WRONLY);
    if (active_low == -1) {
        syslog(LOG_ERR, "gpio%i: mode: Failed to open 'active_low' for writing: %s", dev->pin, strerror(errno));
        return XPT_ERROR_INVALID_RESOURCE;
    }

    char bu[MAX_SIZE];
    int length;
    switch (mode) {
        case XPT_GPIO_ACTIVE_HIGH:
            length = snprintf(bu, sizeof(bu), "%d", 0);
            break;
        case XPT_GPIO_ACTIVE_LOW:
            length = snprintf(bu, sizeof(bu), "%d", 1);
            break;
        default:
            close(active_low);
            return XPT_ERROR_FEATURE_NOT_IMPLEMENTED;
    }

    if (write(active_low, bu, length * sizeof(char)) == -1) {
        syslog(LOG_ERR, "gpio%i: mode: Failed to write to 'active_low': %s", dev->pin, strerror(errno));
        close(active_low);
        return XPT_ERROR_INVALID_RESOURCE;
    }

    close(active_low);

    return XPT_SUCCESS;
}


xpt_result_t
xpt_gpio_out_driver_mode(xpt_gpio_context dev, xpt_gpio_out_driver_mode_t mode)
{
    if (dev == NULL) {
        syslog(LOG_ERR, "gpio: write: context is invalid");
        return XPT_ERROR_INVALID_HANDLE;
    }

    if (IS_FUNC_DEFINED(dev, gpio_out_driver_mode_replace)) {
        return dev->advance_func->gpio_out_driver_mode_replace(dev, mode);
    }
    else {
        return XPT_ERROR_FEATURE_NOT_SUPPORTED;
    }
}
