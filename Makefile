#SUBDIRS := $(wildcard */)
MAKEFILES := $(wildcard */Makefile)
#DIRS := $(dir $(MAKEFILES))
DIRS := userspace kernel_module

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	cd $@ && $(MAKE)
	#$(MAKE) -C $@

#userspace:
#	$(MAKE) -C userspace
#
#kernel_module:
#	cd $@ && $(MAKE)

clean:
	for dir in $(DIRS); do \
		cd $$dir && $(MAKE) clean; \
	done

