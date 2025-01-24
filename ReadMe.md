# XGORider - RiderPI Kernel Module

### This is a lightweight kernel module for the xgorider. The provided Python scripts are shitty as hell, slow and dumb.


Features:
* /proc filesystem
* shutdown on low power
* always keep yaw, when move forward/backward
* manage display, buttons and 

## /proc Filesystem
Basedir: /proc/XGORider
In this dir you can find for reading:
* yaw
* state
* buttons
* battery
* buttons


for writing:
* yaw
* leds/1 -4
* action
* settings/shutdown_on_low_batt
* settings/low_batt
* settings/verbose

The values are refreshed in the Background via a Kernel Thread, so no blocking etc.
Values are only refreshed when in balanced mode. If not refreshing is skipped.


## Shutdown on less power
When battery is under 8% the system do poweroff. So no filesystem damage.

## Shutdown on Buttons
You cann shutdown, if you press ButtonA and ButtonB 3 Seconds. 

