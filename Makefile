MAKEFILES := $(wildcard */Makefile)
DIRS := userspace kernel_module st7789/project/raspberrypi4b

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	cd $@ && $(MAKE)

clean:
	for dir in $(DIRS); do \
		cd $$dir && $(MAKE) clean; \
	done
