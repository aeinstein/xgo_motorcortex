#include "comm.h"


void loop() {
    if(read_addr(XGO_BATTERY, 1)) {
        battery = rx_data[0];

        printf("Battery: %d\n", rx_data[0]);

        if (battery < 10) {
            system("sudo poweroff");
        }
    }


    if(operational) {
        // When balancing hold yaw
        if(read_addr(XGO_YAW, 4)) {
            float result = Byte2Float(rx_data);
            result = result - initial_yaw;

            const uint8_t speed = 128 - (int)result - (int)wanted_yaw;

            if(speed > 5) {
                if(verbose) printf("turning: %f - %f = %d\n", wanted_yaw, result, speed);

                unsigned char cmd[] = {speed};
                write_serial_data(XGO_VYAW, cmd, sizeof(cmd));
            }
        }
    }


    // Check state
    if(read_addr(XGO_STATE, 1)) {
        //printf("State: %d\n", rx_data[0]);
        if(rx_data[0] != operational) {
            operational = rx_data[0];
            printf("State changed ");
            switch(operational){
                case 0x00:
                    printf("fallen\n");
                    break;

                case 0x01:
                    printf("balancing\n");
                    read_initial_yaw(); // reset initial yaw, when standup
                    break;

                default:
                    printf("unknown %d fuck the shit docs\n", operational);
                    break;
            }
        }
    }

    /*



    if(read_addr(XGO_PITCH, 4)) {
        const float result = Byte2Float(rx_data);
        printf("PITCH: %f\n", result);
    }

    if(read_addr(XGO_ROLL, 4)) {
        const float result = Byte2Float(rx_data);
        printf("ROLL: %f\n", result);
    }*/
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
                }

                printf("no finish\n");
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

int read_serial_data(const size_t addr, char *buffer, const size_t len) {
    if (serial_port < 0) {
        perror("Serial port not opened");
        return -1;
    }

    if(verbose) printf("read\n");

    const int mode = 0x02;
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

int write_serial_data(const size_t addr, const char * value, const uint8_t len){
    if(verbose) printf("send %d bytes\n", len);

    int value_sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        value_sum += value[i];
    }

    if(verbose) printf("val_sum %d\n", value_sum);

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
        printf("XGORider: len: %lu\n", sizeof(cmd));

        for (int i = 0; i < sizeof(cmd); i++) {
            printf("send 0x%02X \n", cmd[i]);  // %02X sorgt für zweistellige Hex-Werte
        }

        printf("\n");
    }

    write(serial_port, cmd, sizeof(cmd));

    return 0;
}



bool read_addr(const int addr, size_t len){
    char read_buf[len];

    memset(read_buf, 0, sizeof(read_buf));
    const int num_bytes = read_serial_data(addr, read_buf, sizeof(read_buf));

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
    }

    perror("Error reading from serial port");

    return false;
}


float Byte2Float(const uint8_t rawdata[4]) {
    const uint32_t temp = (rawdata[3] << 24) | (rawdata[2] << 16) | (rawdata[1] << 8) | rawdata[0];
    float result;
    memcpy(&result, &temp, sizeof(float));
    return result;
}

int main() {
    open_serial_port();

    read_addr(XGO_FIRMWARE_VERSION, 10);
    printf("firmware version: %s\n", rx_data);


	read_initial_yaw();


    unsigned char cmd[] = {0x01};
    write_serial_data(XGO_ACTION, cmd, sizeof(cmd));
	
	
	sleep(5);
	
	unsigned char cmd2[] = {115};
    write_serial_data(XGO_BODYHEIGHT, cmd2, sizeof(cmd2));

    sleep(1);

    cmd2[0] = 75;
    write_serial_data(XGO_BODYHEIGHT, cmd2, sizeof(cmd2));
    sleep(1);

    wanted_yaw = 180;

    while(1){
        loop();
        //sleep(5);
        usleep(100000);
    }
}

bool read_initial_yaw() {
    if(read_addr(XGO_YAW, 4)) {
        const float result = Byte2Float(rx_data);
        printf("initial yaw: %f\n", result);
        initial_yaw = result;
        wanted_yaw = 0;
		return 1;
    }
	
	return 0;
}

bool open_serial_port(){
	serial_port = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);
    if (serial_port < 0) {
        perror("Error opening serial port");
        return 0;
    }

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        perror("Error getting terminal attributes");
        close(serial_port);
        return 0;
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
        return 0;
    }

    if(verbose) printf("setted\n");
    return 1;
}
