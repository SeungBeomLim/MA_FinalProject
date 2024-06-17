#ifndef LED_H
#define LED_H

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#define LEFT 0
#define RIGHT 1

#define LED_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(holtek_ht16k33)
#define KEY_NODE DT_CHILD(LED_NODE, keyscan)

#define MAX_LED_NUM 128

#define MAX_ROTARY_IDX 10 // ?창�궗쨘횂혨?횇쩐횏흹 15????횂혶횇���

static const struct device *const led = DEVICE_DT_GET(LED_NODE);

int led_init(void);

void led_off_all(void);
void led_on_idx(int idx, bool right_left);
void led_on_center(void);
void led_on_right(void);
void led_on_left(void);
void led_on_up(void);
void led_on_down(void);

void display_pattern(const uint8_t pattern[8], bool right_left);
void display_success(void);
void display_not_success(void);

extern const uint8_t led_patterns[10][8];

extern const uint8_t password_success[8];
extern const uint8_t password_unsuccess[8];

#endif // LED_H