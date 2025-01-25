//
// Created by aeinstein on 24.01.2025.
//

#include "xgo-serial.h"

// Funktion zum Öffnen der seriellen Schnittstelle
static int open_serial_port(void) {
    pr_info("XGORider: open %s\n", SERIAL_PORT);
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

static bool read_addr(const int addr, size_t len){
    char read_buf[len];

    memset(read_buf, 0, sizeof(read_buf));
    const int num_bytes = read_serial_data(addr, read_buf, sizeof(read_buf));

    if(verbose) pr_info("XGORider: num_bytes: %d", num_bytes);

    if (num_bytes > 0) {
        if(verbose) {
            pr_info("XGORider: Received: %d\n", rx_data[0]);

            for (int i = 0; i < num_bytes; i++) {
                pr_cont("0x%02X ", rx_data[i]);  // %02X sorgt für zweistellige Hex-Werte
            }

            pr_info("");
        }

        return true;
    }

    pr_warn("XGORider: Error reading from serial port");

    return false;
}

static int read_serial_data(size_t addr, char *buffer, size_t len) {
    const int mode = 0x02;
    size_t sum_data = (0x09 + mode + addr + len) % 256;
    sum_data = 255 - sum_data;

    loff_t pos = 0;

    unsigned char cmd[] = {0x55, 0x00, 0x09, mode, addr, len, sum_data, 0x00, 0xAA};

    if(verbose){
    	pr_info( "XGORider: read len: %d\n", sizeof(cmd));

        pr_info( "XGORider: tx_data: ");
    	for (int i = 0; i < sizeof(cmd); i++) {
        	pr_cont("0x%02X ", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
    	}

    	pr_info( "\n");
	}

    kernel_write(serial_file, cmd, sizeof(cmd), &pos);
    if(verbose) pr_info( "XGORider: written %ld bytes\n", sizeof(cmd));

    //msleep(200);

    if(process_data(buffer)) return rx_LEN -8;

    return 0;
}

static int write_serial_data(const size_t addr, char * buffer, const size_t len){
    if(verbose) pr_info( "XGORider: send %d bytes\n", len);

    loff_t pos = 0;

    int value_sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        value_sum += buffer[i];
    }

    if(verbose) pr_info( "XGORider: val_sum %d\n", value_sum);

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
        pr_info( "XGORider: len: %lu\n", sizeof(cmd));

        pr_info( "XGORider: tx_data: ");

        for (int i = 0; i < sizeof(cmd); i++) {
            pr_cont("0x%02X ", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
        }

        pr_info( "\n");
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
                    if(verbose) pr_info( "XGORider: checksum correct\n");
                    rx_FLAG++;
                } else {
                    pr_warn("XGORider: wrong checksum\n");
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
                    pr_warn("XGORider: no finish\n");
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
                        pr_info( "XGORider: rx_data: ");
                        for (size_t j = 0; j < msg_index; j++) {
                            pr_cont("0x%02X ", rx_msg[j]);
                        }
                        pr_info( "\n");

                        pr_info( "XGORider: rxlen: %d\n", rx_LEN);
                    }

                    return true;
                }

                pr_warn("XGORider: no finish\n");
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
