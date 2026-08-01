# libc/Makefile - Builds VulcanOS's libc as a static archive
#
# TOOLCHAIN NOTE: same host-GCC-freestanding approach as
# kernel/Makefile, for the same reason (see the toolchain note
# there) -- this is not hosted userspace code with access to a real
# system's libc; it's freestanding, compiled the same way the
# kernel itself is, since VulcanOS has no ring-3/syscall boundary
# yet for it to run on the other side of (see stdlib.c's file
# comment for the full explanation).
#
# INCLUDE PATH NOTE: libc's own headers (stdio.h, string.h, ...) are
# freestanding and self-contained, but stdio.c/stdlib.c call directly
# into kernel functions (vfs_open, kmalloc, scheduler_current, ...)
# per the interim design documented in stdlib.c -- so this Makefile's
# include path reaches into ../kernel/include as well as libc's own
# include/. This dependency is expected to be REMOVED once a real
# syscall boundary exists; libc calling kernel headers directly is
# specific to this bring-up milestone, not a permanent architecture
# decision.

CC := gcc

BUILD_DIR := build
INCLUDE_DIR := include
KERNEL_INCLUDE_DIR := ../kernel/include

CFLAGS := -m64 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
          -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -mcmodel=large -Wall -Wextra -std=gnu11 \
          -I$(INCLUDE_DIR) -I$(KERNEL_INCLUDE_DIR) -O2 -g

C_SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

LIBC_ARCHIVE := $(BUILD_DIR)/libvulcan.a

.PHONY: all clean

all: $(LIBC_ARCHIVE)

$(LIBC_ARCHIVE): $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
