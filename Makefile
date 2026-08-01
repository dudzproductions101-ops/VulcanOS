# VulcanOS top-level Makefile
#
# Delegates to per-component Makefiles rather than building
# everything from one flat rule set, so each component (kernel,
# bootloader, libc, userland) can be built, tested, and eventually
# packaged independently. Only `kernel` and `iso` currently do real
# work; the rest are wired up but will no-op until their sources
# exist (see PROJECT_STATUS.md for exactly what's implemented).

.PHONY: all kernel iso run clean

all: kernel

kernel:
	$(MAKE) -C kernel

iso: kernel
	mkdir -p iso/boot/grub
	cp kernel/build/vulcanos.elf iso/boot/vulcanos.elf
	grub-mkrescue -o vulcanos.iso iso

run: iso
	qemu-system-x86_64 -cdrom vulcanos.iso -m 512M

clean:
	$(MAKE) -C kernel clean
	rm -f vulcanos.iso
	rm -rf iso/boot/vulcanos.elf
