//
// Created by aeinstein on 23.01.2025.
//
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/serial.h>
#include <linux/fs.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/termios.h>
#include <linux/reboot.h>
#include "constants.h"


#define PROC_DIR "XGORider"
#define SERIAL_PORT "/dev/ttyAMA0" // Anpassen je nach Hardware
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

static struct task_struct *thread;

bool verbose = false;
static bool process_data(char *buffer);
static struct proc_dir_entry *proc_imu, *proc_yaw, *proc_pitch, *proc_roll, *proc_battery;
static struct file *serial_file;
static int read_serial_data(size_t addr, char *buffer, size_t len);
static bool read_addr(const int addr, size_t len);

static int loop(void *data) {
	while (!kthread_should_stop()) {
        if(read_addr(XGO_BATTERY, 1)) {
        	battery = rx_data[0];

        	printk(KERN_INFO "Battery: %d", battery);

        	if (battery < 10) {
                orderly_poweroff(true);
        	}
    	}

		msleep(1000);
	}
	return 0;
}

static bool read_addr(const int addr, size_t len){
    char read_buf[len];

    memset(read_buf, 0, sizeof(read_buf));
    const int num_bytes = read_serial_data(addr, read_buf, sizeof(read_buf));

    if(verbose) printk(KERN_INFO "num_bytes: %d", num_bytes);

    if (num_bytes > 0) {
        if(verbose) {
            printk(KERN_INFO "Received: %d\n", rx_data[0]);

            for (int i = 0; i < num_bytes; i++) {
                printk(KERN_CONT "0x%02X ", rx_data[i]);  // %02X sorgt für zweistellige Hex-Werte
            }

			printk(KERN_INFO "");
        }

        return true;
    }

    printk(KERN_WARNING "Error reading from serial port");

    return false;
}

static int read_serial_data(size_t addr, char *buffer, size_t len) {
    const int mode = 0x02;
    size_t sum_data = (0x09 + mode + addr + len) % 256;
    sum_data = 255 - sum_data;

    loff_t pos = 0;

    unsigned char cmd[] = {0x55, 0x00, 0x09, mode, addr, len, sum_data, 0x00, 0xAA};

    if(verbose){
    	printk(KERN_INFO "XGORider: len: %d\n", sizeof(cmd));

    	for (int i = 0; i < sizeof(cmd); i++) {
        	printk(KERN_INFO "send 0x%02X \n", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
    	}

    	printk(KERN_INFO "\n");
	}

    kernel_write(serial_file, cmd, sizeof(cmd), &pos);
    msleep(200);

    pr_info("Written %ld bytes\n", sizeof(cmd));

    if(process_data(buffer)) return rx_LEN -8;


    return 0;
}

static ssize_t yaw_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
    char buffer[4];

    uint8_t num_bytes = read_serial_data(XGO_YAW, buffer, sizeof(buffer));

    if (num_bytes < 0) return -EIO;

    printk(KERN_INFO "XGORider: yaw read %ld\n", num_bytes);

    return simple_read_from_buffer(user_buf, count, ppos, rx_data, num_bytes);
}

static ssize_t battery_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
	char decimal_buffer[4];
	snprintf(decimal_buffer, sizeof(decimal_buffer), "%d", battery);
    return simple_read_from_buffer(user_buf, count, ppos, decimal_buffer, strlen(decimal_buffer));
}

static ssize_t pitch_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
    char buffer[1];
    if (read_serial_data(XGO_PITCH, buffer, sizeof(buffer)) < 0)
        return -EIO;
    return simple_read_from_buffer(user_buf, count, ppos, buffer, strlen(buffer));
}

static ssize_t roll_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
    char buffer[1];
    if (read_serial_data(XGO_ROLL, buffer, sizeof(buffer)) < 0)
        return -EIO;
    return simple_read_from_buffer(user_buf, count, ppos, buffer, strlen(buffer));
}


static const struct proc_ops yaw_ops = {
    .proc_read = yaw_read,
};

static const struct proc_ops pitch_ops = {
    .proc_read = pitch_read,
};

static const struct proc_ops roll_ops = {
    .proc_read = roll_read,
};

static const struct proc_ops battery_ops = {
    .proc_read = battery_read,
};

static bool process_data(char *buffer) {
    size_t msg_index = 0;
    uint8_t rx_CHECK = 0;

    while(1){
        size_t num_bytes = kernel_read(serial_file, buffer, 1, 0);

        for (size_t i = 0; i < num_bytes; i++) {
            uint8_t num = buffer[i];
            rx_msg[msg_index++] = num;

            switch(rx_FLAG){
            case 0:
                if(num == 0x55) rx_FLAG++;
                break;

            case 1:
                if(num == 0x00) rx_FLAG++;
                break;

            case 2:
                rx_LEN = num;
                rx_FLAG++;
                break;

            case 3:
                rx_TYPE = num;
                rx_FLAG++;
                break;

            case 4:
                rx_ADDR = num;
                rx_COUNT = 0;
                rx_FLAG++;
                break;

            case 5:
                if(rx_COUNT == (rx_LEN - 9)) {
                    rx_data[rx_COUNT] = num;
                    rx_FLAG++;

                } else if (rx_COUNT < rx_LEN - 9) {
                    rx_data[rx_COUNT++] = num;
                }
                break;

            case 6:
                rx_CHECK = 0;

                for (size_t j = 0; j < rx_LEN - 8; j++) {
                    uint8_t t = rx_data[j];
                    rx_CHECK = rx_CHECK +t;
                }

                rx_CHECK = 255 - ((rx_LEN + rx_TYPE + rx_ADDR + rx_CHECK) % 256);

                if (num == rx_CHECK) {
                    if(verbose) printk(KERN_INFO "checksum correct\n");
                    rx_FLAG++;
                } else {
                    printk(KERN_WARNING "wrong checksum\n");
                    rx_FLAG = 0;
                    rx_COUNT = 0;
                    rx_ADDR = 0;
                    rx_LEN = 0;
                    return false;
                }
                break;

            case 7:
                if(num == 0x00){
                    rx_FLAG++;
                } else {
                    printk(KERN_WARNING "no finish\n");
                    rx_FLAG = 0;
                    rx_COUNT = 0;
                    rx_ADDR = 0;
                    rx_LEN = 0;
                }
                break;

            case 8:
                if (num == 0xAA) {
                    rx_FLAG = 0;

                    if(verbose) {
                        printk(KERN_INFO "rx_data: ");
                        for (size_t j = 0; j < msg_index; j++) {
                            printk(KERN_INFO "%02X ", rx_msg[j]);
                        }
                        printk(KERN_INFO "\n");

                        printk(KERN_INFO "rxlen: %d\n", rx_LEN);
                    }

                    return true;
                }

                printk(KERN_WARNING "no finish\n");
                rx_FLAG = 0;
                rx_COUNT = 0;
                rx_ADDR = 0;
                rx_LEN = 0;
                break;

            default:
                return false;
            }
        }
    }
}

static int write_serial_data(const size_t addr, const char * value, const uint8_t len){
    if(verbose) printk(KERN_INFO "send %d bytes\n", len);

    int value_sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        value_sum += value[i];
    }

    if(verbose) printk(KERN_INFO "val_sum %d\n", value_sum);

    const int mode = 0x01;
    int sum_data = ((len + 0x08) + mode + addr + value_sum) % 256;
    sum_data = 255 - sum_data;

    unsigned char cmd[len + 0x08];

    cmd[0] = 0x55;
    cmd[1] = 0x00;
    cmd[2] = len + 0x08;
    cmd[3] = mode;
    cmd[4] = addr;

    for (uint8_t i = 0; i < len; i++) {
        cmd[i + 0x05] = value[i];
    }

    cmd[len + 0x05] = sum_data;
    cmd[len + 0x06] = 0x00;
    cmd[len + 0x07] = 0xAA;

    if(verbose) {
        printk(KERN_INFO "XGORider: len: %lu\n", sizeof(cmd));

        for (int i = 0; i < sizeof(cmd); i++) {
            printk(KERN_INFO "send 0x%02X \n", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
        }

        printk(KERN_INFO "\n");
    }

    kernel_write(serial_file, cmd, sizeof(cmd), 0);

    return 0;
}

// Funktion zum Öffnen der seriellen Schnittstelle
static int open_serial_port(void) {
    struct termios term;

    printk(KERN_INFO "XGORider open %s\n", SERIAL_PORT);
    serial_file = filp_open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC, 0);

    if (IS_ERR(serial_file)) {
        pr_err("Failed to open serial device: %ld\n", PTR_ERR(serial_file));
        return PTR_ERR(serial_file);
    }

    pr_info("Serial device opened successfully\n");


    term.c_cflag  = BAUD_RATE | CLOCAL | CREAD; // 115200 if change, must configure scanner

    /* No parity (8N1) */
    term.c_cflag &= ~PARENB;
    term.c_cflag &= ~CSTOPB;
    term.c_cflag &= ~CSIZE;
    term.c_cflag |= CS8;

    term.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    term.c_oflag &= ~OPOST;

    term.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);

    term.c_cc[VTIME] = 5; // 0.5 seconds read timeout
    term.c_cc[VMIN] = 0;  // read does not block

    /*
    if (serial_tty_ioctl(serial_file, TCSETS, (unsigned long)&term) < 0) {
        pr_err("%s: Failed to set termios\n", __FUNCTION__);
        return -1;
    }
*/

    return 0;
}

static float Byte2Float(const uint8_t rawdata[4]) {
    const uint32_t temp = (rawdata[3] << 24) | (rawdata[2] << 16) | (rawdata[1] << 8) | rawdata[0];
    float result;
    memcpy(&result, &temp, sizeof(float));
    return result;
}

static int __init imu_proc_init(void) {

  	printk(KERN_INFO "              ******       ******");
	printk(KERN_INFO "            **********   **********");
	printk(KERN_INFO "          ************* *************");
	printk(KERN_INFO "         ************      ************");
	printk(KERN_INFO "         ************ KERK ************");
	printk(KERN_INFO "         ************      ***********");
	printk(KERN_INFO "          ***************************");
	printk(KERN_INFO "            ***********************");
	printk(KERN_INFO "              *******************");
	printk(KERN_INFO "                ***************");
	printk(KERN_INFO "                  ***********");
	printk(KERN_INFO "                    *******");
	printk(KERN_INFO "                      ***");
	printk(KERN_INFO "                       *");
    printk(KERN_INFO "XGORider init");

    proc_imu = proc_mkdir(PROC_DIR, NULL);
    if (!proc_imu) {
        remove_proc_entry("yaw", proc_imu);
        remove_proc_entry("pitch", proc_imu);
        remove_proc_entry("roll", proc_imu);
        remove_proc_entry("battery", proc_imu);
        remove_proc_entry(PROC_DIR, NULL);
        return -ENOMEM;
    }

    int ret = open_serial_port();

    if (ret) {
        return ret;
    }

    char buffer[10];

    int num_bytes = read_serial_data(XGO_FIRMWARE_VERSION, buffer, sizeof(buffer));

    if(num_bytes < 0) return -EIO;
    printk(KERN_INFO "got: %d bytes\n", num_bytes);
    printk(KERN_INFO "firmware version: %s\n", rx_data);



    //simple_read_from_buffer(user_buf, count, ppos, buffer, strlen(buffer));

    proc_yaw = proc_create("yaw", 0444, proc_imu, &yaw_ops);
    proc_pitch = proc_create("pitch", 0444, proc_imu, &pitch_ops);
    proc_roll = proc_create("roll", 0444, proc_imu, &roll_ops);
    proc_battery = proc_create("battery", 0444, proc_imu, &battery_ops);

    if (!proc_yaw || !proc_pitch || !proc_roll || !proc_battery) return -ENOMEM;

    pr_info("proc bindungs created\n");

    thread = kthread_run(loop, NULL, "my_kthread");
	if (IS_ERR(thread)) {
        pr_err("Fehler beim Starten des Kernel-Threads\n");
        return PTR_ERR(thread);
    }
    return 0;
}

static void __exit imu_proc_exit(void) {
  	if (thread) {
        kthread_stop(thread);
    }

    filp_close(serial_file, NULL);
    remove_proc_entry("yaw", proc_imu);
    remove_proc_entry("pitch", proc_imu);
    remove_proc_entry("roll", proc_imu);
    remove_proc_entry("battery", proc_imu);
    remove_proc_entry(PROC_DIR, NULL);
}

module_init(imu_proc_init);
module_exit(imu_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Balen");
MODULE_DESCRIPTION("XGO IMU Procfs Kernel Module");
