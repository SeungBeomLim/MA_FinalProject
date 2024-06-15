/*
 * Copyright (c) 2022 Valerio Setti <valerio.setti@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include "led.h"
#include "batterydisplay.h"
#include "value.h"

// [LED Part]
#define LEFT 0
#define RIGHT 1

#if !DT_NODE_EXISTS(DT_ALIAS(qdec0))
#error "Unsupported board: qdec0 devicetree alias is not defined"
#endif

#define SW_NODE DT_NODELABEL(gpiosw)
#if !DT_NODE_HAS_STATUS(SW_NODE, okay)
#error "Unsupported board: gpiosw devicetree alias is not defined or enabled"
#endif
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET(SW_NODE, gpios);

#define MAX_SAVED_NUMBERS 4
static bool sw_led_flag = false;
static int rotary_idx = 0;
static int saved_numbers[MAX_SAVED_NUMBERS] = { -1, -1, -1, -1 };
static int saved_index = 0;
static int password[MAX_SAVED_NUMBERS] = {1, 2, 3, 4}; //?òÑ?û¨ Í∏àÍ≥† ÎπÑÎ??Î≤àÌò∏
static bool password_matched = false;
int flag = true; //Í∏àÍ≥† ???Î¶¨Î©¥ falseÎ°? Î≥??ï®. 

static struct gpio_callback sw_cb_data;

// ?ô∏Î∂? ?Ñ†?ñ∏ Ï∂îÍ??
extern const uint8_t led_patterns[10][8];

// [Battery Display Part]
struct k_timer my_timer;
struct k_work my_work;

static int seconds = 180;


// [LED Part]
bool compare_arrays(int *array1, int *array2, int size) { //Í∏àÍ≥† ÎπÑÎ≤àÍ≥? ?òÑ?û¨ ?ûÖ?†•?ïú ÎπÑÎ≤à ?ôï?ù∏
    for (int i = 0; i < size; i++) {
        if (array1[i] != array2[i]) {
            return false;
        }
    }
    return true;
}

void sw_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    printk("SW pressed, displaying number %d on the right matrix\n", rotary_idx);
    sw_led_flag = true;

    // Save the current number
    saved_numbers[saved_index] = rotary_idx;
    saved_index = (saved_index + 1) % MAX_SAVED_NUMBERS;

    // Print saved numbers
    if (saved_index == 0) {  // 4Î≤? encoderÎ•? ?àå????ùÑ ?ñÑ
        printk("complete\n");
        printk("Saved numbers: ");
        for (int i = 0; i < MAX_SAVED_NUMBERS; i++) {
            printk("%d ", saved_numbers[i]);
        }
        printk("\n");
        
        if (compare_arrays(saved_numbers, password, MAX_SAVED_NUMBERS)) {
            printk("Password matched!\n");
            password_matched = true;
        } else {
            printk("Password not matched!\n");
			flag = false;
        }
    }
}

void display_rotary_led(int32_t rotary_val)
{
    if (rotary_val == 0) {
    } else if (rotary_val - 17 > 0) {
        rotary_idx++;
    } else if (rotary_val + 17 < 0) {
        rotary_idx--;
    }

    if (rotary_idx < 0) {
        rotary_idx = MAX_ROTARY_IDX - 1;
    } else if (rotary_idx >= MAX_ROTARY_IDX) {
        rotary_idx = 0;
    }

    printk("Rotary encoder moved, displaying number %d on the left matrix\n", rotary_idx);
    led_on_idx(rotary_idx, LEFT);
}

// [Battery Display Part]

void my_work_handler(struct k_work *work)
{
        printk("Time: %d\n", seconds);

        // Display the time on the battery display
        if (seconds == 180 || seconds >= 175){
                display_level(7);
        } else if(seconds >= 170 && seconds < 175){
                display_level(6);
        } else if(seconds >= 165 && seconds < 170){
                display_level(5);
        } else if(seconds >= 160 && seconds < 165){
                display_level(4);
        } else if(seconds >= 155 && seconds < 160){
                display_level(3);
        } else if(seconds >= 150 && seconds < 155){
                display_level(2);
        } else if(seconds >= 140 && seconds < 150){
                display_level(1);
        } else {
                display_level(0);
        }

        // Increment the seconds
        seconds--;
        if (seconds < 0){
                seconds = 180;
        }
}

K_WORK_DEFINE(my_work, my_work_handler);

void my_timer_handler(struct k_timer *dummy)
{
        k_work_submit(&my_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

int main(void)
{
    struct sensor_value val;
    int rc;
    const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(qdec0));

    int ret = DK_OK;
    ret = batterydisplay_intit();
    if (ret != DK_OK) {
            printk("Error initializing battery display\n");
            return DK_ERR;
        }
    }

    ret = led_init();
    if (ret != DK_OK) {
            printk("Error initializing LED\n");
            return DK_ERR;
        }   
    }

    if (!device_is_ready(dev)) {
        printk("Qdec device is not ready\n");
        return 0;
    }

    if (!device_is_ready(sw.port)) {
        printk("SW GPIO is not ready\n");
        return 0;
    }

    int err = gpio_pin_configure_dt(&sw, GPIO_INPUT | GPIO_PULL_UP);
    if (err < 0) {
        printk("Error configuring SW GPIO pin %d\n", err);
        return 0;
    }

    gpio_init_callback(&sw_cb_data, sw_callback, BIT(sw.pin));
    err = gpio_add_callback(sw.port, &sw_cb_data);
    if (err < 0) {
        printk("Error adding callback for SW GPIO pin %d\n", err);
        return 0;
    }

    err = gpio_pin_interrupt_configure_dt(&sw, GPIO_INT_EDGE_TO_ACTIVE);
    if (err != 0) {
        printk("Error configuring SW GPIO interrupt %d\n", err);
        return 0;
    }

    printk("Quadrature decoder sensor test\n");

    if (led_init() < 0) {
        printk("LED init failed\n");
        return 0;
    }

    led_on_idx(rotary_idx, LEFT);

    while (true) {
        if (password_matched) {
            display_success();
            break; // ÎπÑÎ??Î≤àÌò∏Í∞? ÎßûÏúºÎ©? while ?ÉàÏ∂?
        }

		if (!flag) {
            display_not_success();
            break; // ÎπÑÎ??Î≤àÌò∏Í∞? ???Î¶¨Î©¥ whileÎ¨? ?ÉàÏ∂?. Í∑ºÎç∞ Í≥ÑÏÜç ÎπÑÎ≤à??? ?ûÖ?†•?ï¥?ïº ?ïò?ãà, ?ÉàÏ∂úÏ?? ÎßêÍ≥†, ?ùº?†ï ?ãúÍ∞ÑÎèô?ïà ?ö∞?äî ?ñºÍµ? Î≥¥Ïó¨Ï£ºÍ≥† ?ã§?ãú 0?úºÎ°? Î¶¨ÏÖã?ïò?ì†Ïß? ?ï¥?ïº?ï†?ìØ.
        }

        rc = sensor_sample_fetch(dev);
        if (rc != 0) {
            printk("Failed to fetch sample (%d)\n", rc);
            return 0;
        }

        rc = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
        if (rc != 0) {
            printk("Failed to get data (%d)\n", rc);
            return 0;
        }

        if (!sw_led_flag) {
            display_rotary_led(val.val1);
        } else {
            // Display selected pattern on the right side
            display_pattern(led_patterns[rotary_idx], RIGHT);
            sw_led_flag = false; // Reset flag to allow continuous update
        }

        printk("current value: %d\n", rotary_idx);

        k_msleep(750);
    }

    return 0;
}

void pir() {

}