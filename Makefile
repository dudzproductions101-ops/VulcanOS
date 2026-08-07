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

usb: iso
	@echo "Creating hybrid USB image from vulcanos.iso..."
	cp vulcanos.iso vulcanos-usb.img
	if command -v isohybrid >/dev/null 2>&1; then \
		isohybrid vulcanos-usb.img || true; \
	fi
	@echo "USB image ready: vulcanos-usb.img (dd to a USB device to boot on real hardware)"

run: iso
	qemu-system-x86_64 -cdrom vulcanos.iso -m 512M

clean:
	$(MAKE) -C kernel clean
	rm -f vulcanos.iso
	rm -rf iso/boot/vulcanos.elf
