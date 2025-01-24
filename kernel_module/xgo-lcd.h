//
// Created by aeinstein on 24.01.2025.
//

#ifndef XGO_LCD_H
#define XGO_LCD_H

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/spi/spidev.h>

#include "xgo-gpio.h"

#define SPI_PORT "/dev/spidev0.0"
#define SPI_SPEED 40000000
#define I2C_SPEED 100000

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

static struct file *spi_device;
static struct gpio_desc *rst, *dc, *bl;
struct spi_ioc_transfer xfer[2];

int display_init(void);
void display_command(uint8_t command, char * data, uint8_t len);

void display_reset(void);

#endif //XGO_LCD_H
