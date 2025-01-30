//
// Created by aeinstein on 23.01.2025.
//
#include "xgo-drv.h"
#include "xgo-gpio.h"
#include "xgo-proc.c"
#include "xgo-serial.c"

union B2I16 conv;

static int loop(void *data) {
	pr_info("XGORider: loop start");

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
        if(verbose & VERBOSE_MAIN) pr_info("XGORider: turning: %d - %d = %d\n", wanted_yaw, current_yaw, speed);
        unsigned char cmd[] = {speed};
        write_serial_data(XGO_VYAW, cmd, sizeof(cmd));
    }
}

static int16_t readYaw(void){
    if(read_addr(XGO_YAW_INT, 2)) {
        conv.b[0] = rx_data[1];
        conv.b[1] = rx_data[0];
        current_yaw = conv.i;

        if(verbose & VERBOSE_MAIN) pr_info("XGORider: current yaw: %d", current_yaw);
        return current_yaw;
    }

    return 0;
}

static bool read_initial_yaw() {
    wanted_yaw = 0;
    initial_yaw = readYaw();
    pr_info("XGORider: initial yaw: %f", initial_yaw);

    return 0;
}


static void batteryCheck(void) {
    if(read_addr(XGO_BATTERY, 1)) {
        battery = rx_data[0];

        if(verbose & VERBOSE_MAIN) pr_info("XGORider: Battery: %d", battery);

        if (battery < XGO_LOW_BATT) {
          	pr_warn("XGORider: Battery: %d, force shutdown", battery);
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
            pr_info("XGORider: State changed");

            switch(operational){
                case 0x00:
                    pr_warn("XGORider: fallen");
                    break;

                case 0x01:
                    pr_info("XGORider: balancing");
                    read_initial_yaw(); // reset initial yaw, when standup
                    break;

                default:
                    printk(KERN_ERR "XGORider: unknown %d fuck the shit docs\n", operational);
                    break;
            }
        }
    }
}

// Not working, so many tries, hope you got more luck
/*
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

    pr_info("Einstellungen für %s erfolgreich geändert.\n", tty_name);
}*/

static int __init imu_proc_init(void) {
  	pr_info("              ******       ******");
	pr_info("            **********   **********");
	pr_info("          ************* *************");
	pr_info("         ************      ************");
	pr_info("         ************ KERK ************");
	pr_info("         ************      ***********");
	pr_info("          ***************************");
	pr_info("            ***********************");
	pr_info("              *******************");
	pr_info("                ***************");
	pr_info("                  ***********");
	pr_info("                    *******");
	pr_info("                      ***");
	pr_info("                       *");
    pr_info("XGORider init");



    int ret = open_serial_port();
    if (ret) return ret;

    char buffer[10];
    int num_bytes = read_serial_data(XGO_FIRMWARE_VERSION, buffer, sizeof(buffer));

    if(num_bytes < 0) return -EIO;
    pr_info("XGORider: firmware version: %s\n", rx_data);

    ret = initGPIO();
    if(ret) return -EIO;

    //ret = display_init();
    //if(ret) return ret;

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
    stop_read_loop();

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
