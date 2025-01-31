# XGORider - RiderPI Software

 
This is a lightweight kernel module for the xgorider. The provided Python scripts are shitty as hell, slow and dumb.
And yes I know that accessing a tty via /dev is not the 'right' way !!! Right way is to develop a platform driver. You are free to do.

The Software ist divided into parts:
* The Kernel Module
* The Display Driver
* The Remote Control Host
* The Remote Control Client
* RTMP Server for low latency video

## The Kernel Module

Features:
* /proc filesystem
* shutdown on low power
* always keep yaw, when move forward/backward
* manage buttons 

## /proc Filesystem
Basedir: /proc/XGORider

In this dir you can find for reading:
* yaw
* state
* buttons 1=A, 2=B, 3= A+B etc
* battery in %



for writing:

| file     | format   | description              |
|----------|----------|--------------------------|
| action   | Number   | Start specified action   |
| yaw      | Number   | Get Yaw                  |
| 

* action - 0-255 refer manual
* yaw
* roll -
* height - 75 - 115
* leds/0 - #RRGGBB RGB Value in Hex 
* leds/1 - #RRGGBB RGB Value in Hex
* leds/2 - #RRGGBB RGB Value in Hex
* leds/3 - #RRGGBB RGB Value in Hex
* settings/shutdown_on_low_batt - 1, shutdown system when batt is under low_batt
* settings/low_batt - low watermark in %
* settings/verbose - see verbose level
* settings/sleep_ms_on_loop - ms sleep in main loop
* settings/force_yaw - 1 = hold yaw, under all circumstances 
* settings/calibration - 1 = start

The values are refreshed in the Background via a Kernel Thread, so no blocking etc.
Values are only refreshed when in balanced mode. If not only state is refreshing.

### verbose level
bitwise
* 1 serial
* 2 gpio
* 4 proc
* 8 main

### Calibration
see [https://wiki.elecfreaks.com/en/microbit/robot/xgo-rider-kit/cases-libraries/case08-calibration-mode]

### important
Because of security, you can't set baudrate in kernel driver.
you have to set it via crontab, on boot


### Shutdown on less power
When battery is under low_batt the system do poweroff. So no filesystem damage.

### Shutdown on Buttons
You can shutdown, if you press Button A and Button B 3 Seconds. 

### create test image
hexdump -s 70 -v -e '16/1 "0x%02X, " "\n"' mundk.bmp > we


## The Display Driver
It is a userspace daemon which creates a fifo /tmp/lcd0

there you can send raw bitmap RGB565 

