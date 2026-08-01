# user/Makefile - Builds VulcanOS's userland as a static archive
#
# Same architecture as libc/Makefile (host-GCC freestanding, same
# reasoning) and the same interim reality: these are not yet
# separate executables loaded from disk into isolated ring-3
# address spaces -- they're real, standalone C programs, compiled
# into their own objects and linked into the kernel image, each
# started as its own scheduler thread (see the file comments in
# user/include/syscall.h and init/init.c for the full explanation).
# Building this as a static archive (like libc) rather than compiling
# these files directly into the kernel's own build keeps userland's
# build boundary explicit and visually distinct from kernel/libc,
# even though today they all end up in one linked ELF regardless.

CC := gcc

BUILD_DIR := build
INCLUDE_DIR := include
SHELL_INCLUDE_DIR := shell
INIT_INCLUDE_DIR := init
LIBC_INCLUDE_DIR := ../libc/include
KERNEL_INCLUDE_DIR := ../kernel/include

CFLAGS := -m64 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
          -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -mcmodel=large -Wall -Wextra -std=gnu11 \
          -I$(INCLUDE_DIR) -I$(SHELL_INCLUDE_DIR) -I$(INIT_INCLUDE_DIR) \
          -I$(LIBC_INCLUDE_DIR) -I$(KERNEL_INCLUDE_DIR) -O2 -g

C_SOURCES := $(shell find bin init shell -name '*.c' 2>/dev/null)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

USER_ARCHIVE := $(BUILD_DIR)/libvulcan_user.a

.PHONY: all clean

all: $(USER_ARCHIVE)

$(USER_ARCHIVE): $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
