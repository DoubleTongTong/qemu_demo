BASEINCLUDE ?= /lib/modules/`uname -r`/build

mypci-objs := pci_demo.o
obj-m := mypci.o

all:
	$(MAKE) -C $(BASEINCLUDE) M=$(PWD) modules;

clean:
	$(MAKE) -C $(BASEINCLUDE) M=$(PWD) clean;
	rm -rf *.ko;