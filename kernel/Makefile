# kernel/Makefile - Builds the VulcanOS kernel ELF
#
# TOOLCHAIN NOTE: this uses the host system's gcc/ld in freestanding
# mode rather than a true x86_64-elf-* cross-compiler. On an x86_64
# host targeting an x86_64 kernel, host GCC with -ffreestanding
# produces the same result a cross-compiler would, since there's no
# host/target ISA mismatch to cross. This is a documented, standard
# fallback (see OSDev wiki's "Building a Cross-Compiler" vs. host-GCC
# freestanding notes) -- NOT the long-term correct setup. Building a
# real x86_64-elf-gcc via crosstool-ng is recommended before this
# kernel is ever built on a non-x86_64 host (e.g. Apple Silicon),
# since host-GCC freestanding only works because host==target here.

CC      := gcc
LD      := ld
NASM    := nasm

BUILD_DIR := build
INCLUDE_DIR := include
LIBC_DIR := ../libc
LIBC_INCLUDE_DIR := $(LIBC_DIR)/include
LIBC_ARCHIVE := $(LIBC_DIR)/build/libvulcan.a
USER_DIR := ../user
USER_INCLUDE_DIR := $(USER_DIR)/init
USER_ARCHIVE := $(USER_DIR)/build/libvulcan_user.a

CFLAGS := -m64 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
          -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -mcmodel=large -Wall -Wextra -std=gnu11 \
          -I$(INCLUDE_DIR) -I$(LIBC_INCLUDE_DIR) -I$(USER_INCLUDE_DIR) -O2 -g

LDFLAGS := -nostdlib -static -T linker.ld -z max-page-size=0x1000

ASFLAGS := -f elf64

# Only picks up directories with real implementations. mm/, proc/,
# and fs/ were added once their sources were written (see
# PROJECT_STATUS.md). libc/ and user/ are now real dependencies,
# each built via its own Makefile (see the libc/user targets below)
# and linked in as static archives rather than compiled inline here
# -- each has its own build rules (see libc/Makefile's and
# user/Makefile's toolchain notes, same reasoning as this file's).
C_SOURCES := $(shell find arch core drivers mm proc fs -name '*.c' 2>/dev/null)
ASM_SOURCES := $(shell find arch proc -name '*.asm' 2>/dev/null)

C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

KERNEL_ELF := $(BUILD_DIR)/vulcanos.elf

.PHONY: all clean libc user

all: $(KERNEL_ELF)

libc:
	$(MAKE) -C $(LIBC_DIR)

user:
	$(MAKE) -C $(USER_DIR)

$(KERNEL_ELF): $(OBJECTS) $(LIBC_ARCHIVE) $(USER_ARCHIVE) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(USER_ARCHIVE) $(LIBC_ARCHIVE)

$(LIBC_ARCHIVE): libc
$(USER_ARCHIVE): user

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) $(ASFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(LIBC_DIR) clean
	$(MAKE) -C $(USER_DIR) clean
