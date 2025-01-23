CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = comm
SRCS = comm.c
OBJS = $(SRCS:.c=.o)

obj-m += xgo-drv.o

all: $(TARGET) kernel_module

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)


kernel_module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

