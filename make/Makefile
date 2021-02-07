##############################################################################
BUILD = build
BIN = dgw
PORT = /dev/serial/by-id/usb-DNBDMR_TinyUSB_D11D_CEF3EF1D514D4B53334A2020FF071B46-if00

##############################################################################
.PHONY: all directory clean size fdfu dump

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

#CFLAGS += -Wextra -Wall --std=gnu11 -DDEBUG -ggdb3 -Og
CFLAGS += -Wextra -Wall --std=gnu11 -Os
CFLAGS += -Wno-maybe-uninitialized
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -funsigned-char -funsigned-bitfields
CFLAGS += -mcpu=cortex-m0plus -mthumb
CFLAGS += -MD -MP -MT $(BUILD)/$(*F).o -MF $(BUILD)/$(@F).d

LDFLAGS += -mcpu=cortex-m0plus -mthumb
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--start-group -lm -Wl,--end-group
LDFLAGS += -Wl,-Map=$(BUILD)/$(BIN).map
LDFLAGS += -Wl,--script=../linker/samd11c14.ld

#LIBS = --specs=nano.specs -u _printf_float -u _scanf_float

INCLUDES += \
  -I../tinyusb/hw/mcu/microchip/samd11/include/ \
  -I../tinyusb/hw/mcu/microchip/samd11/CMSIS/Core/Include/ \
  -I../tinyusb/src \
  -I..

SRCS += \
  ../main.c \
  ../startup_samd11.c \
  ../tinyusb/src/tusb.c \
  ../tinyusb/src/common/tusb_fifo.c \
  ../tinyusb/src/device/usbd.c \
  ../tinyusb/src/device/usbd_control.c \
  ../tinyusb/src/portable/microchip/samd/dcd_samd.c \
  ../tinyusb/src/class/cdc/cdc_device.c \
  ../usb_descriptors.c \
  ../light_ws2812_cortex.c

DEFINES += \
  -D__SAMD11D14AM__ \
  -DDONT_USE_CMSIS_INIT \
  -DF_CPU=48000000 \
  -DSTARTUP_FROM_RESET \
  -DLIGHT_WS2812_UC_SAMD \
  -DLIGHT_WS2812_GPIO_PORT=0 \
  -DLIGHT_WS2812_GPIO_PIN=2

CFLAGS += $(INCLUDES) $(DEFINES)

OBJS = $(addprefix $(BUILD)/, $(notdir %/$(subst .c,.o, $(SRCS))))

all: $(BUILD)/$(BIN).elf size

$(BUILD)/$(BIN).elf: $(OBJS)
	@echo LD $@
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(BUILD)/$(BIN).hex: $(BUILD)/$(BIN).elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O ihex $^ $@

$(BUILD)/$(BIN).bin: $(BUILD)/$(BIN).elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O binary $^ $@

$(BUILD)/$(BIN).dfu: $(BUILD)/$(BIN).elf
	@echo DFU $@
	./dx1elf2dfu  $^ $@

fdfu: $(BUILD)/$(BIN).dfu
	@echo Flash DFU $^
	-./force_reset.py --period 0.5 $(PORT)
	dfu-util -D $^

disasm: $(BUILD)/$(BIN).elf
	@echo DISASSEMBLE $^
	$(OBJDUMP) -d -S $^ > $(BUILD)/$(BIN).lss

$(OBJS): | $(BUILD)
	@echo CC $@
	@$(CC) $(CFLAGS) $(filter %/$(subst .o,.c,$(notdir $@)), $(SRCS)) -c -o $@

$(BUILD):
	@mkdir -p $(BUILD)

size: $(BUILD)/$(BIN).elf
	@echo size:
	@$(SIZE) -t $^

clean:
	@echo clean
	@-rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)
