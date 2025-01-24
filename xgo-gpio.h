//
// Created by aeinstein on 24.01.2025.
//

#ifndef XGO_GPIO_H
#define XGO_GPIO_H

#include <linux/gpio/consumer.h>
#include "xgo-drv.h"

static struct gpio_desc *button_a, *button_b, *button_c, *button_d;

static void gpioCheck(void){
	XGO_Buttons = 0;
	if(!gpiod_get_value(button_a)) XGO_Buttons += 1;
	if(!gpiod_get_value(button_b)) XGO_Buttons += 2;
	if(!gpiod_get_value(button_c)) XGO_Buttons += 4;
	if(!gpiod_get_value(button_d)) XGO_Buttons += 8;

	if(verbose) printk(KERN_INFO "GPIO check: %d", XGO_Buttons);

	if(XGO_Buttons == 3) orderly_poweroff(true);
}

static int initGPIO(void){
	int status = 0;
	button_a = gpio_to_desc(GPIO_BUTTON_A + GPIO_OFFSET);
	button_b = gpio_to_desc(GPIO_BUTTON_B + GPIO_OFFSET);
	button_c = gpio_to_desc(GPIO_BUTTON_C + GPIO_OFFSET);
	button_d = gpio_to_desc(GPIO_BUTTON_D + GPIO_OFFSET);

	gpiod_is_active_low(button_a);

	if(!button_a || !button_b || !button_c || !button_d){
		printk(KERN_WARNING "Error creating GPIO descriptors\n");
		return -ENODEV;
	}

	status = gpiod_direction_input(button_a);
	if(status) {
		printk(KERN_WARNING "Error setting GPIO direction input\n");
		return status;
	}

	status = gpiod_direction_input(button_b);
	if(status) {
		printk(KERN_WARNING "Error setting GPIO direction input\n");
		return status;
	}

	status = gpiod_direction_input(button_c);
	if(status) {
		printk(KERN_WARNING "Error setting GPIO direction input\n");
		return status;
	}

	status = gpiod_direction_input(button_d);
	if(status) {
		printk(KERN_WARNING "Error setting GPIO direction input\n");
		return status;
	}

	return 0;
}

#endif //XGO_GPIO_H
