#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "constants.h"

#define BAUD_RATE B115200

#define MAX_DATA_LEN 256

uint8_t rx_FLAG = 0;
uint8_t rx_LEN = 0;
uint8_t rx_TYPE = 0;
uint8_t rx_ADDR = 0;
uint8_t rx_COUNT = 0;
uint8_t rx_data[MAX_DATA_LEN];
uint8_t rx_msg[MAX_DATA_LEN];
bool verbose = false;
int serial_port;

bool read_addr(int addr, size_t len);

float Byte2Float(uint8_t rawdata[4]) {
    uint32_t temp = (rawdata[3] << 24) | (rawdata[2] << 16) | (rawdata[1] << 8) | rawdata[0];
    float result;
    memcpy(&result, &temp, sizeof(float));
    return result;
}

bool process_data(char *buffer) {
    size_t msg_index = 0;
    uint8_t rx_CHECK = 0;

    while(1){
        size_t num_bytes = read(serial_port, buffer, 1);

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
                    if(verbose) printf("checksum correct\n");
                    rx_FLAG++;
                } else {
                    printf("wrong checksum\n");
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
                    printf("no finish\n");
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
                        printf("rx_data: ");
                        for (size_t j = 0; j < msg_index; j++) {
                            printf("%02X ", rx_msg[j]);
                        }
                        printf("\n");
                    }

                    return true;

                } else {
                    printf("no finish\n");
                    rx_FLAG = 0;
                    rx_COUNT = 0;
                    rx_ADDR = 0;
                    rx_LEN = 0;
                }
                break;
            }
        }
    }

    return false;
}

int read_serial_data(size_t addr, char *buffer, size_t len) {
    if (serial_port < 0) {
        perror("Serial port not opened");
        return -1;
    }

    if(verbose) printf("read\n");

    int mode = 0x02;
    size_t sum_data = (0x09 + mode + addr + len) % 256;
    sum_data = 255 - sum_data;

    unsigned char cmd[] = {0x55, 0x00, 0x09, 0x02, addr, len, sum_data, 0x00, 0xAA};
    if(verbose) {
        printf("XGORider: len: %lu\n", sizeof(cmd));

        for (int i = 0; i < sizeof(cmd); i++) {
            printf("send 0x%02X \n", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
        }

        printf("\n");
    }

    write(serial_port, cmd, sizeof(cmd));

    if(process_data(buffer)) return rx_LEN -8;

    return 0;
}

/*
int write_serial_data(size_t addr, char *value){
    int len = sizeof(value);

    int mode = 0x01;
    int sum_data = ((len + 0x08) + mode + addr + value_sum) % 256;
    sum_data = 255 - sum_data;



    unsigned char cmd[len + 0x08] = {0x55, 0x00, (len + 0x08), mode, addr};



    cmd[len + 0x06] = sum_data;
    cmd[len + 0x07] = 0x00;
    cmd[len + 0x08] = 0xAA;



    if(verbose) {
        printf("XGORider: len: %d\n", sizeof(cmd));

        for (int i = 0; i < sizeof(cmd); i++) {
            printf("send 0x%02X \n", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
        }

        printf("\n");
    }

    write(serial_port, cmd, sizeof(cmd));

    return 0;
}
*/

int main() {
    serial_port = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);
    if (serial_port < 0) {
        perror("Error opening serial port");
        return 1;
    }

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        perror("Error getting terminal attributes");
        close(serial_port);
        return 1;
    }

    printf("opened\n");

    cfsetispeed(&tty, BAUD_RATE);
    cfsetospeed(&tty, BAUD_RATE);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        perror("Error setting terminal attributes");
        close(serial_port);
        return 1;
    }

    if(verbose) printf("setted\n");

    read_addr(XGO_FIRMWARE_VERSION, 10);
    printf("firmware version: %s\n", rx_data);

    while(1){
        if(read_addr(XGO_BATTERY, 1)) printf("Battery: %d\n", rx_data[0]);

        if(read_addr(XGO_YAW, 4)) {
            float result = Byte2Float(rx_data);
            printf("YAW: %f\n", result);
        }

        if(read_addr(XGO_PITCH, 4)) {
            float result = Byte2Float(rx_data);
            printf("PITCH: %f\n", result);
        }

        if(read_addr(XGO_ROLL, 4)) {
            float result = Byte2Float(rx_data);
            printf("ROLL: %f\n", result);
        }

        sleep(1);
    }

    return 0;
}

bool read_addr(int addr, size_t len){
    char read_buf[len];

    memset(read_buf, 0, sizeof(read_buf));
    int num_bytes = read_serial_data(addr, read_buf, sizeof(read_buf));

    if(verbose) printf("num_bytes: %d\n", num_bytes);

    if (num_bytes > 0) {
        if(verbose) {
            printf("Received: %d\n", rx_data[0]);

            for (int i = 0; i < num_bytes; i++) {
                printf("got 0x%02X \n", rx_data[i]);  // %02X sorgt für zweistellige Hex-Werte
            }

            printf("\n");
        }

        return true;

    } else {
        perror("Error reading from serial port");
    }

    return false;
}


