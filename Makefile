.PHONY: all clean prog openocd

APP_DIR := .

ifeq ($(QMSI_DIR),)
$(error QMSI_DIR is not defined)
endif

ifeq ($(IAMCU_TOOLCHAIN_DIR),)
$(error IAMCU_TOOLCHAIN_DIR is not defined)
endif

APP_NAME := test
APP_OBJECTS := test.o
CFLAGS += -O -Wall -g

OPENOCD := $(IAMCU_TOOLCHAIN_DIR)/../../debugger/openocd/bin/openocd
OPENOCD_CFG := openocd.cfg

SYS_SRC := $(QMSI_DIR)/sys
SYS_OBJECTS := sys/app_entry.o sys/newlib-syscalls.o
OBJECTS := $(APP_OBJECTS) $(SYS_OBJECTS)

LDSCRIPT := $(QMSI_DIR)/soc/quark_d2000/quark_d2000.ld

CROSS := $(IAMCU_TOOLCHAIN_DIR)/i586-intel-elfiamcu-
CC := $(CROSS)gcc
LD := $(CROSS)ld
SIZE := $(CROSS)size
OBJCOPY := $(CROSS)objcopy

# This must match what was used to build QMSI.
CFLAGS += -ffunction-sections -fdata-sections -Os -fomit-frame-pointer -std=c99
CFLAGS += -DPRINTF_ENABLE -DPUTS_ENABLE

# QMSI include paths.
CFLAGS += -I$(QMSI_DIR)/include
CFLAGS += -I$(QMSI_DIR)/drivers/include
CFLAGS += -I$(QMSI_DIR)/soc/quark_d2000/include

LDFLAGS += -L$(QMSI_DIR)/build/release/quark_d2000/x86/libqmsi/lib
LDFLAGS += -nostdlib -Xlinker --gc-sections
LDFLAGS += -Xlinker -T$(LDSCRIPT)
LIBS += -lqmsi

all: $(APP_NAME).bin

clean:
	rm -f $(BIN) $(ELF) $(OBJECTS)

$(APP_NAME).elf:	$(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

%.bin:	%.elf
	$(SIZE) $<
	$(OBJCOPY) -O binary $< $@

# Build a local copy of some QMSI files that depend on the application's needs.
sys/%.o:	$(SYS_SRC)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

prog:	$(APP_NAME).bin
	$(OPENOCD) -f $(OPENOCD_CFG) -c "flash_and_run $<;shutdown"

openocd:
	$(OPENOCD) -f $(OPENOCD_CFG)
