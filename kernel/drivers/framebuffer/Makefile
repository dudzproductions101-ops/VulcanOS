CC := gcc

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
    -I../../include \
    -I../../include/drivers \
    -O2 \
    -g

BUILD_DIR := build

C_SOURCES := $(wildcard *.c)

OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

all: $(OBJECTS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
