#ARCH: linux/pi/android/ios/
ARCH		?= linux
CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar

CC_V	= $(CC)
CXX_V	= $(CXX)
LD_V	= $(LD)
AR_V	= $(AR)
CP_V	= $(CP)
RM_V	= $(RM)

###############################################################################
# target and object
###############################################################################
LIBNAME		= libcommbus
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_LIB_SO_VER	= $(TGT_LIB_SO).${VER}

TGT_LIB_H	= 
		  
LIBI2C_O	= src/i2c/i2c.o
LIBSPI_O	= src/spi/spi.o
LIBUART_O	= src/uart/uart.o
LIBIIO_O	= src/iio/iio.o
		  
LIBPWM_O	= src/pwm/pwm.o
		  
		  
		  
LIBUARTOW_O	= src/uart_ow/uart_ow.o
LIBGPIO_O   = src/gpio/gpio.o 
		  
		  
		  
LIBAIO_O	= src/aio/aio.o
LIBMIPS_O	= src/mips/mediatek.o \
			  src/mips/mips.o 



OBJS_LIB	= $(LIBI2C_O) \
		  $(LIBSPI_O) \
		  $(LIBUART_O) \
		  $(LIBIIO_O) \
		  $(LIBPWM_O) \
		  $(LIBUARTOW_O) \
		  $(LIBGPIO_O) \
		  $(LIBAIO_O) \
		  $(LIBMIPS_O) 
		 


###############################################################################
# cflags and ldflags
###############################################################################
CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -Iinclude/ \
		   -Iapi/xpt \
		   -Iapi\
	   
SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread -ldl -lrt

###############################################################################
# target
###############################################################################
.PHONY : all clean

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)

OBJS	:= $(OBJS_LIB) $(OBJS_UNIT_TEST)

all: $(TGT)

%.o:%.c
	$(CC_V) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR_V) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(CC_V) -o $@ $^ $(SHARED)
	@mv $(TGT_LIB_SO) $(TGT_LIB_SO_VER)
	@ln -sf $(TGT_LIB_SO_VER) $(TGT_LIB_SO)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST) $(ANDROID_MAIN_OBJ)
	$(CC_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)
	$(RM_V) -f version.h
	$(RM_V) -f $(TGT_LIB_SO)*
	$(RM_V) -f $(TGT_LIB_SO_VER)

install:
	$(CP_V) -r $(TGT_LIB_H)  $(OUTPUT)/include/
	$(CP_V) -r $(TGT_LIB_A)  $(OUTPUT)/lib/
	$(CP_V) -r $(TGT_LIB_SO) $(OUTPUT)/lib/
	$(CP_V) -r $(TGT_LIB_SO_VER) $(OUTPUT)/lib/

uninstall:
	$(RM_V) -f $(OUTPUT)/include/$(LIBNAME)/$(TGT_LIB_H)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_A)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO_VER)
