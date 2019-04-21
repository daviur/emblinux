ifneq ($(KERNELRELEASE),)
obj-m := nunchuk.o
else
KDIR := /home/andrew/emblinux-projects/linux
all:
	$(MAKE) -C $(KDIR) M=$$PWD
endif
