//
// Created by aeinstein on 23.01.2025.
//

#ifndef COMM_H
#define COMM_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "gpiolib.h"
#include "../constants.h"

#define BAUD_RATE B115200
#define MAX_DATA_LEN 32

float initial_yaw = 0;
float wanted_yaw = 0;
int battery = 100;

uint8_t rx_FLAG = 0;
uint8_t rx_LEN = 0;
uint8_t rx_TYPE = 0;
uint8_t rx_ADDR = 0;
uint8_t rx_COUNT = 0;
uint8_t rx_data[MAX_DATA_LEN];
uint8_t rx_msg[MAX_DATA_LEN];
uint8_t operational = 0x01;

bool verbose = false;
int serial_port;

int write_serial_data(size_t addr, const char * value, uint8_t len);

bool read_addr(int addr, size_t len);
float Byte2Float(const uint8_t rawdata[4]);
bool open_serial_port();
bool read_initial_yaw();

#endif //COMM_H
