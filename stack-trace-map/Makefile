KERNEL ?= $(shell readlink -f ~/src/bpf-next)

obj-m := deep-stack.o

.phony: all clean

all:
	make -C $(KERNEL) M=$(shell pwd) modules

clean:
	make -C $(KERNEL) M=$(shell pwd) clean
