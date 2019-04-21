ifneq ($(KERNELRELEASE),)
obj-m := hello_version.o
else
KDIR := /home/andrew/emblinux-projects/linux
all:
	$(MAKE) -C $(KDIR) M=$$PWD
endif
