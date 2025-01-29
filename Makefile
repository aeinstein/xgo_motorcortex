MAKEFILES := $(wildcard */Makefile)

DIRS := kernel_module st7789/project/riderpi
#DIRS := userspace kernel_module st7789/project/riderpi

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	cd $@ && $(MAKE)

clean:
	for dir in $(DIRS); do \
		cd $$dir && $(MAKE) clean; \
	done

install:
	for dir in $(DIRS); do \
		cd $$dir && $(MAKE) install; \
	done


