
TARGET ?= qemu

all:  bootloader kernel userland toolchain image

bootloader:
	cd Bootloader; make all

kernel:
	cd Kernel; make all

userland:
	cd Userland; make all

toolchain:
	cd Toolchain; make all

image: kernel bootloader userland toolchain
	$(MAKE) -C Image TARGET=$(TARGET) all

# Convenience targets for explicit images
image-qemu: TARGET=qemu
image-qemu: image

image-vbox: TARGET=vbox
image-vbox: image

image-usb: TARGET=usb
image-usb: image

clean:
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Toolchain; make clean
	cd Userland; make clean

.PHONY: bootloader image collections kernel userland toolchain all clean