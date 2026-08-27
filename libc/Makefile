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
