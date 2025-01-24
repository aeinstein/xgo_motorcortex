//
// Created by aeinstein on 23.01.2025.
//
#include "xgo-drv.h"
#include "xgo-gpio.h"
#include "xgo-proc.h"

union B2I16 conv;



static int loop(void *data) {
	printk(KERN_INFO "XGORider: loop start");

	while (!kthread_should_stop()) {
	    gpioCheck();
	    batteryCheck();
        checkState();

        if(operational == 0x01) {
            readYaw();

            if(XGO_HOLD_YAW) forceyaw();






        }

		msleep(XGO_MS_SLEEP_ON_LOOP);
	}
	return 0;
}

static void forceyaw(){
	const uint8_t speed = 128 - (current_yaw - wanted_yaw);

	if(speed > 5) {
        if(verbose) printk(KERN_INFO "XGORider: turning: %d - %d = %d\n", wanted_yaw, current_yaw, speed);
        unsigned char cmd[] = {speed};
        write_serial_data(XGO_VYAW, cmd, sizeof(cmd));
    }
}

static int16_t readYaw(void){
    if(read_addr(XGO_YAW_INT, 2)) {
        conv.b[0] = rx_data[1];
        conv.b[1] = rx_data[0];
        current_yaw = conv.i;

        if(verbose) printk(KERN_INFO "XGORider: current yaw: %d", current_yaw);
        return current_yaw;
    }

    return 0;
}

static bool read_initial_yaw() {
    wanted_yaw = 0;
    initial_yaw = readYaw();
    printk(KERN_INFO "XGORider: initial yaw: %f", initial_yaw);

    return 0;
}


static void batteryCheck(void) {
    if(read_addr(XGO_BATTERY, 1)) {
        battery = rx_data[0];

        if(verbose) printk(KERN_INFO "XGORider: Battery: %d", battery);

        if (battery < XGO_LOW_BATT) {
          	printk(KERN_WARNING "XGORider: Battery: %d, force shutdown", battery);
            orderly_poweroff(true);
        }
    }
}

static void checkState(void) {
    // Check state
    if(read_addr(XGO_STATE, 1)) {
        //printf("State: %d\n", rx_data[0]);
        if(rx_data[0] != operational) {
            operational = rx_data[0];
            printk(KERN_INFO "XGORider: State changed");

            switch(operational){
                case 0x00:
                    printk(KERN_WARNING "XGORider: fallen");
                    break;

                case 0x01:
                    printk(KERN_INFO "XGORider: balancing");
                    read_initial_yaw(); // reset initial yaw, when standup
                    break;

                default:
                    printk(KERN_ERR "XGORider: unknown %d fuck the shit docs\n", operational);
                    break;
            }
        }
    }
}

static bool read_addr(const int addr, size_t len){
    char read_buf[len];

    memset(read_buf, 0, sizeof(read_buf));
    const int num_bytes = read_serial_data(addr, read_buf, sizeof(read_buf));

    if(verbose) printk(KERN_INFO "XGORider: num_bytes: %d", num_bytes);

    if (num_bytes > 0) {
        if(verbose) {
            printk(KERN_INFO "XGORider: Received: %d\n", rx_data[0]);

            for (int i = 0; i < num_bytes; i++) {
                printk(KERN_CONT "0x%02X ", rx_data[i]);  // %02X sorgt für zweistellige Hex-Werte
            }

			printk(KERN_INFO "");
        }

        return true;
    }

    printk(KERN_WARNING "XGORider: Error reading from serial port");

    return false;
}



static int read_serial_data(size_t addr, char *buffer, size_t len) {
    const int mode = 0x02;
    size_t sum_data = (0x09 + mode + addr + len) % 256;
    sum_data = 255 - sum_data;

    loff_t pos = 0;

    unsigned char cmd[] = {0x55, 0x00, 0x09, mode, addr, len, sum_data, 0x00, 0xAA};

    if(verbose){
    	printk(KERN_INFO "XGORider: read len: %d\n", sizeof(cmd));

        printk(KERN_INFO "XGORider: tx_data: ");
    	for (int i = 0; i < sizeof(cmd); i++) {
        	printk(KERN_CONT "0x%02X ", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
    	}

    	printk(KERN_INFO "\n");
	}

    kernel_write(serial_file, cmd, sizeof(cmd), &pos);
    if(verbose) printk(KERN_INFO "XGORider: written %ld bytes\n", sizeof(cmd));

    //msleep(200);

    if(process_data(buffer)) return rx_LEN -8;

    return 0;
}

static int write_serial_data(const size_t addr, char * buffer, const size_t len){
    if(verbose) printk(KERN_INFO "XGORider: send %d bytes\n", len);

    loff_t pos = 0;

    int value_sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        value_sum += buffer[i];
    }

    if(verbose) printk(KERN_INFO "XGORider: val_sum %d\n", value_sum);

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
        cmd[i + 0x05] = buffer[i];
    }

    cmd[len + 0x05] = sum_data;
    cmd[len + 0x06] = 0x00;
    cmd[len + 0x07] = 0xAA;

    if(verbose) {
        printk(KERN_INFO "XGORider: len: %lu\n", sizeof(cmd));

        printk(KERN_INFO "XGORider: tx_data: ");

        for (int i = 0; i < sizeof(cmd); i++) {
            printk(KERN_CONT "0x%02X ", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
        }

        printk(KERN_INFO "\n");
    }

    kernel_write(serial_file, cmd, sizeof(cmd), &pos);

    return 0;
}

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
                    if(verbose) printk(KERN_INFO "XGORider: checksum correct\n");
                    rx_FLAG++;
                } else {
                    printk(KERN_WARNING "XGORider: wrong checksum\n");
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
                    printk(KERN_WARNING "XGORider: no finish\n");
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
                        printk(KERN_INFO "XGORider: rx_data: ");
                        for (size_t j = 0; j < msg_index; j++) {
                            printk(KERN_CONT "0x%02X ", rx_msg[j]);
                        }
                        printk(KERN_INFO "\n");

                        printk(KERN_INFO "XGORider: rxlen: %d\n", rx_LEN);
                    }

                    return true;
                }

                printk(KERN_WARNING "XGORider: no finish\n");
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

// Not working, so many tries, hope you got more luck
static void modify_serial_port_settings(const char *tty_name, int baudrate){
    struct tty_struct *tty;
    struct ktermios new_termios;

//#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
    //tty = serial_file->private_data;
//#else
    tty = (struct tty_struct *)serial_file->private_data;
//#endif

    if (!tty) {
        printk(KERN_ERR "XGORider: TTY-Gerät konnte nicht erhalten werden.");
        return;
    }

    // Sperren des TTYs während der Modifikation
    tty_lock(tty);
    new_termios = tty->termios;

    tty_termios_encode_baud_rate(&new_termios, baudrate, baudrate);
    tty_set_termios(tty, &new_termios);
    tty_unlock(tty);

    printk(KERN_INFO "Einstellungen für %s erfolgreich geändert.\n", tty_name);
}

// Funktion zum Öffnen der seriellen Schnittstelle
static int open_serial_port(void) {
    struct ktermios new_settings;

    printk(KERN_INFO "XGORider: open %s\n", SERIAL_PORT);
    serial_file = filp_open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC, 0);

    if (IS_ERR(serial_file)) {
        pr_err("XGORider: Failed to open serial device: %ld\n", PTR_ERR(serial_file));
        return PTR_ERR(serial_file);
    }

    pr_info("Serial device opened successfully\n");

    //modify_serial_port_settings("ttyAMA0", B115200);


    /*
    term.c_cflag  = BAUD_RATE | CLOCAL | CREAD; // 115200 if change, must configure scanner

    // No parity (8N1)
    term.c_cflag &= ~PARENB;
    term.c_cflag &= ~CSTOPB;
    term.c_cflag &= ~CSIZE;
    term.c_cflag |= CS8;

    term.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    term.c_oflag &= ~OPOST;

    term.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);

    term.c_cc[VTIME] = 5; // 0.5 seconds read timeout
    term.c_cc[VMIN] = 0;  // read does not block


    if (serial_tty_ioctl(serial_file, TCSETS, (unsigned long)&term) < 0) {
        pr_err("%s: Failed to set termios\n", __FUNCTION__);
        return -1;
    }
*/

    return 0;
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

    int ret = open_serial_port();
    if (ret) return ret;

    char buffer[10];
    int num_bytes = read_serial_data(XGO_FIRMWARE_VERSION, buffer, sizeof(buffer));

    if(num_bytes < 0) return -EIO;
    //printk(KERN_INFO "XGORider: got: %d bytes\n", num_bytes);
    printk(KERN_INFO "XGORider: firmware version: %s\n", rx_data);

    ret = initGPIO();
    if(ret) return -EIO;

    ret = createFilesystem();
    if(ret) return ret;

    thread = kthread_run(loop, NULL, "my_kthread");
	if(IS_ERR(thread)) {
        pr_err("XGORider: Fehler beim Starten des Kernel-Threads\n");
        return PTR_ERR(thread);
    }
    return 0;
}

static void __exit imu_proc_exit(void) {
  	if (thread) {
        kthread_stop(thread);
    }

    filp_close(serial_file, NULL);
    destroyFilesystem();
}

module_init(imu_proc_init);
module_exit(imu_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Balen");
MODULE_DESCRIPTION("XGORider IMU Procfs Kernel Module");
