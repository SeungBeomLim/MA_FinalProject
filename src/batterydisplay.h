#ifndef BATTERYDISPLAY_H
#define BATTERYDISPLAY_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#define DISPLAY_BRIGHTEST 0x88
#define BRIGHTNESS_LEVEL1 0x01

#define ADDR_FIXED 0x44
#define ADDR_CMD_00H 0xC0

int batterydisplay_init(void);
void set_brightness(int brightness);
int display_level(uint8_t level);
void display_clear(void);

void start(void);
void stop(void);
void write_byte(int8_t wr_data);
void bit_delay(void);

#endif // BATTERYDISPLAY_H
