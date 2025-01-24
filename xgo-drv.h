//
// Created by aeinstein on 24.01.2025.
//

#ifndef XGO_DRV_H
#define XGO_DRV_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/serial.h>
#include <linux/fs.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/termios.h>
#include <linux/reboot.h>

static bool verbose = true;


#include "constants.h"

#define PROC_DIR "XGORider"
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

uint8_t leds[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};


static uint8_t XGO_Buttons = 0;

static bool XGO_HOLD_YAW = false;
static uint8_t XGO_LOW_BATT = 8;
static bool XGO_SHUTDOWN_ON_LOW_BATT = true;


static struct task_struct *thread;

static int initGPIO(void);

static bool process_data(char *buffer);

static struct file *serial_file;
static int read_serial_data(size_t addr, char *buffer, size_t len);
static int write_serial_data(size_t addr, char *buffer, size_t len);
static bool read_addr(const int addr, size_t len);
static bool read_initial_yaw(void);
static void gpioCheck(void);
static void batteryCheck(void);
static void checkState(void);
static int16_t readYaw(void);
static int createFilesystem(void);
static void destroyFilesystem(void);
static int16_t readYaw(void);
static void forceyaw(void);

union B2I16 {
	int16_t i;
	char    b[2];
};

#endif //XGO_DRV_H
