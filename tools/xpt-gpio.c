#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xpt/gpio.h"

struct gpio_source {
    int pin;
    xpt_gpio_context context;
};

void
print_version()
{
    fprintf(stdout, "Version %s on %s\n", xpt_get_version(), xpt_get_platform_name());
}

void
print_help()
{
    fprintf(stdout, "list              List pins\n");
    fprintf(stdout, "set pin level     Set pin to level (0/1)\n");
    fprintf(stdout, "setraw pin level  Set pin to level (0/1) via mmap (if available)\n");
    fprintf(stdout, "get pin           Get pin level\n");
    fprintf(stdout, "getraw pin        Get pin level via mmap (if available)\n");
    fprintf(stdout, "monitor pin       Monitor pin level changes\n");
    fprintf(stdout, "version           Get xpt version and board name\n");
}

void
print_command_error()
{
    fprintf(stdout, "Invalid command, options are:\n");
    print_help();
}

int
list_platform_pins(uint8_t platform_offset)
{
    int pin_count = xpt_get_platform_pin_count(platform_offset);
    int i;
    for (i = 0; i < pin_count; ++i) {
        int pin_id = platform_offset > 0 ? xpt_get_sub_platform_id(i) : i;
        char* pin_name = xpt_get_pin_name(pin_id);
        if (strcmp(pin_name, "INVALID")  != 0 && xpt_pin_mode_test(pin_id, XPT_PIN_VALID)) {
            fprintf(stdout, "%02d ", pin_id);
            fprintf(stdout, "%*s: ", (XPT_PIN_NAME_SIZE - 1), pin_name);
            if (xpt_pin_mode_test(pin_id, XPT_PIN_GPIO))
                fprintf(stdout, "GPIO ");
            if (xpt_pin_mode_test(pin_id, XPT_PIN_I2C))
                fprintf(stdout, "I2C  ");
            if (xpt_pin_mode_test(pin_id, XPT_PIN_SPI))
                fprintf(stdout, "SPI  ");
            if (xpt_pin_mode_test(pin_id, XPT_PIN_PWM))
                fprintf(stdout, "PWM  ");
            if (xpt_pin_mode_test(pin_id, XPT_PIN_AIO))
                fprintf(stdout, "AIO  ");
            if (xpt_pin_mode_test(pin_id, XPT_PIN_UART))
                fprintf(stdout, "UART ");
            fprintf(stdout, "\n");
        }
    }
    return pin_count;    
}

int
list_pins()
{
    int pin_count = 0;
    pin_count += list_platform_pins(XPT_MAIN_PLATFORM_OFFSET);
    pin_count += list_platform_pins(XPT_SUB_PLATFORM_OFFSET);
    if (pin_count == 0) {
        fprintf(stdout, "No Pins\n");
    }
    return pin_count;
}

xpt_result_t
gpio_set(int pin, int level, xpt_boolean_t raw)
{
    xpt_gpio_context gpio;
    if (raw) {
        gpio = xpt_gpio_init_raw(pin);
    }
    else {
        gpio = xpt_gpio_init(pin);
    }
    if (gpio != NULL) {
        xpt_gpio_dir(gpio, XPT_GPIO_OUT);
        if (raw != 0) {
            if (xpt_gpio_use_mmaped(gpio, 1) != XPT_SUCCESS) {
                fprintf(stdout,
                        "mmapped access to gpio %d not supported, falling back to normal mode\n", pin);
            }
        }
        xpt_gpio_write(gpio, level);
        return XPT_SUCCESS;
    }
    return XPT_ERROR_INVALID_RESOURCE;
}

xpt_result_t
gpio_get(int pin, int* level, xpt_boolean_t raw)
{
    xpt_gpio_context gpio = xpt_gpio_init(pin);
    if (gpio != NULL) {
        xpt_gpio_dir(gpio, XPT_GPIO_IN);
        if (raw != 0) {
            if (xpt_gpio_use_mmaped(gpio, 1) != XPT_SUCCESS) {
                fprintf(stdout,
                        "mmapped access to gpio %d not supported, falling back to normal mode\n", pin);
            }
        }
        *level = xpt_gpio_read(gpio);
        return XPT_SUCCESS;
    }
    return XPT_ERROR_INVALID_RESOURCE;
}


void
gpio_isr_handler(void* args)
{
    struct gpio_source* gpio_info = (struct gpio_source*) args;
    int level = xpt_gpio_read(gpio_info->context);
    fprintf(stdout, "Pin %d = %d\n", gpio_info->pin, level);
}

xpt_result_t
gpio_isr_start(struct gpio_source* gpio_info)
{
    gpio_info->context = xpt_gpio_init(gpio_info->pin);
    if (gpio_info->context != NULL) {
        xpt_result_t status = xpt_gpio_dir(gpio_info->context, XPT_GPIO_IN);
        if (status == XPT_SUCCESS) {
            status = xpt_gpio_isr(gpio_info->context, XPT_GPIO_EDGE_BOTH, &gpio_isr_handler, gpio_info);
        }
        return status;
    } else {
        return XPT_ERROR_INVALID_RESOURCE;
    }
}


xpt_result_t
gpio_isr_stop(struct gpio_source* gpio_info)
{
    xpt_gpio_isr_exit(gpio_info->context);
    xpt_gpio_close(gpio_info->context);
    return XPT_SUCCESS;
}


int
main(int argc, char** argv)
{
    if (argc == 1) {
        print_command_error();
    }

    if (argc > 1) {
        if (strcmp(argv[1], "list") == 0) {
            list_pins();
        } else if (strcmp(argv[1], "help") == 0) {
            print_help();
        } else if (strcmp(argv[1], "version") == 0) {
            print_version();
        } else if ((strcmp(argv[1], "set") == 0) || (strcmp(argv[1], "setraw") == 0)) {
            if (argc == 4) {
                int pin = atoi(argv[2]);
                xpt_boolean_t rawmode = strcmp(argv[1], "setraw") == 0;
                if (gpio_set(pin, atoi(argv[3]), rawmode) != XPT_SUCCESS)
                    fprintf(stdout, "Could not initialize gpio %d\n", pin);
            } else {
                print_command_error();
            }
        } else if ((strcmp(argv[1], "get") == 0) || (strcmp(argv[1], "getraw") == 0)) {
            if (argc == 3) {
                int pin = atoi(argv[2]);
                int level;
                xpt_boolean_t rawmode = strcmp(argv[1], "getraw") == 0;
                if (gpio_get(pin, &level, rawmode) == XPT_SUCCESS) {
                    fprintf(stdout, "Pin %d = %d\n", pin, level);
                } else {
                    fprintf(stdout, "Could not initialize gpio %d\n", pin);
                }
            } else {
                print_command_error();
            }
        } else if (strcmp(argv[1], "monitor") == 0) {
            if (argc == 3) {
                int pin = atoi(argv[2]);
                struct gpio_source gpio_info;
                gpio_info.pin = pin;
                if (gpio_isr_start(&gpio_info) == XPT_SUCCESS) {
                    fprintf(stdout, "Monitoring level changes to pin %d. Press RETURN to exit.\n", pin);
                    gpio_isr_handler(&gpio_info);
                    char aux = 0;
                    do {
                        fflush(stdin);
                        fscanf(stdin, "%c", &aux);
                    } while (aux != '\n');
                    gpio_isr_stop(&gpio_info);
                } else {
                    fprintf(stdout, "Failed to register ISR for pin %d\n", pin);
                }
            } else {
                print_command_error();
            }
        } else {
            print_command_error();
        }
    }
    return 0;
}
