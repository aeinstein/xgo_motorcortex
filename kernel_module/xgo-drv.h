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
#include <linux/version.h>
#include <linux/reboot.h>
#include <inttypes.h>
#include <stdbool.h>

// Enable with: echo 1 > /proc/XGORider/settings/verbose
static bool verbose = false;

#include "../constants.h"

#define PROC_DIR "XGORider"


uint8_t leds[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};


static uint8_t XGO_Buttons = 0;

// Settings
static bool XGO_HOLD_YAW = false;
static uint8_t XGO_LOW_BATT = 8;
static bool XGO_SHUTDOWN_ON_LOW_BATT = true;
static int XGO_MS_SLEEP_ON_LOOP = 200;

static struct task_struct *thread;

static bool read_initial_yaw(void);
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
