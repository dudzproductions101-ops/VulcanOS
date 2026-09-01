CC := gcc
AS := as

BUILD_DIR := build
INCLUDE_DIR := include
KERNEL_INCLUDE_DIR := ../kernel/include

CFLAGS := -m64 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
          -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -mcmodel=large -Wall -Wextra -std=gnu11 \
          -I$(INCLUDE_DIR) -I$(KERNEL_INCLUDE_DIR) -O2 -g

ASFLAGS := -msyntax=intel

C_SOURCES := $(wildcard *.c)
C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
S_OBJECTS := build/graphics_syscall.o
OBJECTS := $(C_OBJECTS) $(S_OBJECTS)

LIBC_ARCHIVE := $(BUILD_DIR)/libvulcan.a

.PHONY: all clean

all: $(LIBC_ARCHIVE)

$(LIBC_ARCHIVE): $(OBJECTS)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/graphics_syscall.o: graphics_syscall.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
