//
// Created by aeinstein on 24.01.2025.
//

#include "xgo-lcd.h"

int display_init(void){
	unsigned char cmd[32];

	rst = gpio_to_desc(GPIO_LCD_RST + GPIO_OFFSET);
	dc = gpio_to_desc(GPIO_LCD_DC + GPIO_OFFSET);
	bl = gpio_to_desc(GPIO_LCD_BL + GPIO_OFFSET);

	gpiod_direction_output(rst, 0);
	gpiod_direction_output(dc, 0);
	gpiod_direction_output(bl, 0);

    spi_device = filp_open(SPI_PORT, O_RDWR, 0);

	if (IS_ERR(spi_device)) {
		pr_err("XGORider: Failed to open serial device: %ld\n", PTR_ERR(spi_device));
		return PTR_ERR(spi_device);
	}

	pr_info("SPI device opened successfully\n");

    display_reset();

	cmd[0] = 0x00;
	display_command(0x36, cmd, 1);

	cmd[0] = 0x05;
	display_command(0x3A, cmd, 1);

	display_command(0x21, cmd, 0);

	cmd[0] = 0x00;
	cmd[1] = 0x00;
	cmd[2] = 0x01;
	cmd[3] = 0x3f;
	display_command(0x2a, cmd, 4);

	cmd[0] = 0x00;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0xef;
	display_command(0x2a, cmd, 4);

	cmd[0] = 0x0c;
	cmd[1] = 0x0c;
	cmd[2] = 0x00;
	cmd[3] = 0x33;
	cmd[4] = 0x33;
	display_command(0xb2, cmd, 5);

	cmd[0] = 0x35;
	display_command(0xb7, cmd, 1);

	cmd[0] = 0x1f;
	display_command(0xbb, cmd, 1);

	cmd[0] = 0x2c;
	display_command(0xc0, cmd, 1);

	cmd[0] = 0x01;
	display_command(0xc2, cmd, 1);

	cmd[0] = 0x12;
	display_command(0xc3, cmd, 1);

	cmd[0] = 0x20;
	display_command(0xc4, cmd, 1);

    cmd[0] = 0x0f;
	display_command(0xc6, cmd, 1);


	cmd[0] = 0xa4;
	cmd[1] = 0xa1;
	display_command(0xd0, cmd, 2);

	cmd[0] = 0xd0;
	cmd[1] = 0x08;
	cmd[2] = 0x11;
	cmd[3] = 0x08;
	cmd[4] = 0x0c;
	cmd[5] = 0x15;
	cmd[6] = 0x39;
	cmd[7] = 0x33;
	cmd[8] = 0x50;
	cmd[9] = 0x36;
	cmd[10] = 0x13;
	cmd[11] = 0x14;
	cmd[12] = 0x29;
	cmd[13] = 0x2d;
	display_command(0xe0, cmd, 14);

	cmd[0] = 0xd0;
	cmd[1] = 0x08;
	cmd[2] = 0x10;
	cmd[3] = 0x08;
	cmd[4] = 0x06;
	cmd[5] = 0x06;
	cmd[6] = 0x39;
	cmd[7] = 0x44;
	cmd[8] = 0x51;
	cmd[9] = 0x0b;
	cmd[10] = 0x16;
	cmd[11] = 0x14;
	cmd[12] = 0x2F;
	cmd[13] = 0x31;
	display_command(0xe1, cmd, 14);

	display_command(0x21, cmd, 0);
	display_command(0x11, cmd, 0);
	display_command(0x29, cmd, 0);

    return 0;
}

void display_command(uint8_t command, char * data, uint8_t length){
  	gpiod_set_value(dc, 0);

    loff_t pos = 0;

    char cmd[] = { command };
	kernel_write(spi_device, cmd, 1, &pos);

    if(length > 0){
        gpiod_set_value(dc, 1);
    	kernel_write(spi_device, data, length, &pos);
    }
}

void display_reset(){
	gpiod_set_value(rst, 1);
	msleep(10);
	gpiod_set_value(rst, 0);
	msleep(10);
	gpiod_set_value(rst, 1);
}
