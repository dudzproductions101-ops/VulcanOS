CC := gcc
LD := ld
NASM := nasm

BUILD_DIR := build

INCLUDE_DIR := include

LIBC_DIR := ../libc
LIBC_INCLUDE_DIR := $(LIBC_DIR)/include
LIBC_ARCHIVE := $(LIBC_DIR)/build/libvulcan.a

USER_DIR := ../user
USER_INCLUDE_DIR := $(USER_DIR)/init
USER_ARCHIVE := $(USER_DIR)/build/libvulcan_user.a


CFLAGS := -m64 \
	-ffreestanding \
	-fno-pic \
	-fno-pie \
	-fno-stack-protector \
	-mno-red-zone \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mcmodel=large \
	-Wall \
	-Wextra \
	-std=gnu11 \
	-I$(INCLUDE_DIR) \
	-I$(LIBC_INCLUDE_DIR) \
	-I$(USER_INCLUDE_DIR) \
	-O2 \
	-g


LDFLAGS := -nostdlib -static -T linker.ld -z max-page-size=0x1000

ASFLAGS := -f elf64


C_SOURCES := $(sort $(shell find arch core drivers mm proc fs pkg -name '*.c' 2>/dev/null) $(wildcard *.c))

ASM_SOURCES := $(sort $(shell find arch proc -name '*.asm' 2>/dev/null))


C_OBJECTS := $(sort $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES)))

ASM_OBJECTS := $(sort $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES)))


OBJECTS := $(sort $(ASM_OBJECTS) $(C_OBJECTS))


KERNEL_ELF := $(BUILD_DIR)/vulcanos.elf


.PHONY: all clean libc user


all: $(KERNEL_ELF)


libc:
	$(MAKE) -C $(LIBC_DIR)


user:
	$(MAKE) -C $(USER_DIR)


$(KERNEL_ELF): $(OBJECTS) $(LIBC_ARCHIVE) $(USER_ARCHIVE) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(USER_ARCHIVE) $(LIBC_ARCHIVE)


$(LIBC_ARCHIVE):
	$(MAKE) -C $(LIBC_DIR)


$(USER_ARCHIVE):
	$(MAKE) -C $(USER_DIR)


$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	$(NASM) $(ASFLAGS) $< -o $@


clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(LIBC_DIR) clean
	$(MAKE) -C $(USER_DIR) clean