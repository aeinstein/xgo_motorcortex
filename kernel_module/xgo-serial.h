//
// Created by aeinstein on 24.01.2025.
//

#ifndef XGO_SERIAL_H
#define XGO_SERIAL_H

#include <linux/serial.h>
#include <linux/fs.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/delay.h>

#include "../constants.h"

#include "xgo-drv.h"

#define SERIAL_PORT "/dev/ttyAMA0" // Anpassen je nach Hardware
#define BAUD_RATE B115200
#define MAX_DATA_LEN 32

int16_t initial_yaw = 0;
int16_t wanted_yaw = 0;
int16_t current_yaw = 0;

uint8_t battery = 100;
uint8_t operational = 0x01;

uint8_t rx_FLAG = 0;
uint8_t rx_LEN = 0;
uint8_t rx_TYPE = 0;
uint8_t rx_ADDR = 0;
uint8_t rx_COUNT = 0;

uint8_t rx_data[MAX_DATA_LEN];
uint8_t rx_msg[MAX_DATA_LEN];

static bool process_data(char *buffer);
static struct file *serial_file;
static int read_serial_data(size_t addr, char *buffer, size_t len);
static int write_serial_data(size_t addr, char *buffer, size_t len);
static bool read_addr(int addr, size_t len);

#endif //XGO_SERIAL_H
