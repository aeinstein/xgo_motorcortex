MAKEFILES := $(wildcard */Makefile)
DIRS := userspace kernel_module

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	cd $@ && $(MAKE)

clean:
	for dir in $(DIRS); do \
		cd $$dir && $(MAKE) clean; \
	done
