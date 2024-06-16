/*
 * Copyright (c) 2022 Valerio Setti <valerio.setti@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "led.h"
#include "batterydisplay.h"

// [LED Part]
#if !DT_NODE_EXISTS(DT_ALIAS(qdec0))
#error "Unsupported board: qdec0 devicetree alias is not defined"
#endif

#define SW_NODE DT_NODELABEL(gpiosw)
#if !DT_NODE_HAS_STATUS(SW_NODE, okay)
#error "Unsupported board: gpiosw devicetree alias is not defined or enabled"
#endif
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET(SW_NODE, gpios);

static int rotary_idx = 0;

#define MAX_SAVED_NUMBERS 4
static bool sw_led_flag = false;
static int saved_numbers[MAX_SAVED_NUMBERS] = { -1, -1, -1, -1 };
static int saved_index = 0;
static int password[MAX_SAVED_NUMBERS] = {1, 2, 3, 4}; //?��?�� 금고 비�??번호
static bool password_matched = false;
int flag = true; //금고 ???리면 false�????? �??????��. 

static struct gpio_callback sw_cb_data;

// ?���????? ?��?�� 추�??
extern const uint8_t led_patterns[10][8];

// [Joystic Part]
static int saved_number_joystick[MAX_SAVED_NUMBERS] = { -1, -1, -1, -1 };
static int password_joystick[MAX_SAVED_NUMBERS] = {1, 2, 3, 4}; // Password of joystick
static int saved_index_joystick = 0;
int flag_joystick = false;

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

// add for joystick
int32_t preX = 0 , perY = 0;
static const int ADC_MAX = 1023;
// static const int ADC_MIN = 0;
static const int AXIS_DEVIATION = ADC_MAX / 2;
int32_t nowX = 0, nowY = 0;

// [LED Part]
bool compare_arrays(int *array1, int *array2, int size) {
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
    saved_numbers[saved_index++] = rotary_idx;
    //saved_index = (saved_index + 1) % MAX_SAVED_NUMBERS;

    // Print saved numbers
    if (saved_index == MAX_SAVED_NUMBERS) {  // 4�????? encoder�????? ?��????�� ?��
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

// [Joystick Part]
bool isChange(void)
{
	if ((nowX < (preX - 50)) || nowX > (preX+50)) {
		preX = nowX;
		return true;
	}

	if ((nowY < (perY - 50)) || nowY > (perY+50)) {
		perY = nowY;
		return true;
	}
    
	return false;
}

// [Battery Display Part]
static int seconds = 120;

// ë°°í??�°ë�?�??? ë ?�ë²�? í??�œì�?��? í??�¨�??��?
void update_battery_display(void)
{
    //int level;
    uint8_t level;

    // ë°°í??�°ë�?�??? ë ?�ë²¨�???? ì´?�ì�?��? ë??�°ë�? ë§¤í??��??
    if (seconds >= 120) {
        level = 10;
    } else if (seconds >= 108) {
        level = 9;
    } else if (seconds >= 96) {
        level = 8;
    } else if (seconds >= 84) {
        level = 7;
    } else if (seconds >= 72) {
        level = 6;
    } else if (seconds >= 60) {
        level = 5;
    } else if (seconds >= 48) {
        level = 4;
    } else if (seconds >= 36) {
        level = 3;
    } else if (seconds >= 24) {
        level = 2;
    } else if (seconds > 12) {
        level = 1;
    } else if (seconds == 0) {
        level = 0;
    }

    // í??�´ë�?��? ë°°í??�°ë�?�??? ë ?�ë²�? í??�œì�?��?
    display_level(level);

    // ì´?? ê°ì??��?
    seconds--;
    if (seconds < 0) {
        //seconds = 11; ë¦¬ì??��?�ë�?
        flag = false;
    }
}

int main(void)
{
    // [LED Part Initialize]
    struct sensor_value val;
    int rc;
    const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(qdec0));

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

    // [Joystick Part Initialize]
    uint32_t count = 0;
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

    /* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}

    if (led_init() < 0) {
        printk("LED init failed\n");
        return 0;
    }
  
    // ë°°í??�°ë�?�??? ë??��?��?Š¤í??�Œë ˆ�?´ ì´?�ê¸°�??��???
    if (batterydisplay_init() < 0) {
        printk("Battery display init failed\n");
        return 0;
    }

    while (1) {
		printk("ADC reading[%u]: ", count++);

		(void)adc_sequence_init_dt(&adc_channels[0], &sequence);
		err = adc_read(adc_channels[0].dev, &sequence);
		if (err < 0) {
			printk("Could not read (%d)\n", err);
			continue;
		}

		nowX = (int32_t)buf;

		(void)adc_sequence_init_dt(&adc_channels[1], &sequence);
		err = adc_read(adc_channels[1].dev, &sequence);
		if (err < 0) {
			printk("Could not read (%d)\n", err);
			continue;
		}

		nowY = (int32_t)buf;

        printk("Joy X: %" PRIu32 ", ", nowX);
		printk("Joy Y: %" PRIu32 ", ", nowY);

		if (nowX >= 65500 || nowY >= 65500){
			printk("Out of Range\n");
			k_sleep(K_MSEC(100));
			continue;
		}

		bool checkFlag = isChange();
		if(!checkFlag){
            printk("No Change\n");
			k_sleep(K_MSEC(100));
			continue;
		} else {
			led_off_all();
		}

		if (nowX == ADC_MAX && nowY == ADC_MAX){
			led_on_center();
            flag_joystick = true;
			printk("Center");
		} else if (nowX < AXIS_DEVIATION && nowY == ADC_MAX){
			led_on_left();
            if (flag_joystick) {
                saved_number_joystick[saved_index_joystick++] = 4;
            }
            flag_joystick = false;
			printk("Left");
		} else if (nowX > AXIS_DEVIATION && nowY == ADC_MAX){
			led_on_right();
            if (flag_joystick) {
                saved_number_joystick[saved_index_joystick++] = 2;
            }
            flag_joystick = false;
			printk("Right");
		} else if (nowY > AXIS_DEVIATION && nowX == ADC_MAX){
			led_on_up();
            if (flag_joystick) {
                saved_number_joystick[saved_index_joystick++] = 1;
            }
            flag_joystick = false;
			printk("Up");
		} else if (nowY < AXIS_DEVIATION && nowX == ADC_MAX){
			led_on_down();
            if (flag_joystick) {
                saved_number_joystick[saved_index_joystick++] = 3;
            }
            flag_joystick = false;
			printk("Down");
		}
        
        if (saved_index_joystick == MAX_SAVED_NUMBERS) {
            if (compare_arrays(saved_number_joystick, password_joystick, MAX_SAVED_NUMBERS)) {
                led_off_all();
                display_success();
                k_msleep(3000);
                led_off_all();
                break;
            }
            else {
                led_off_all();
                display_not_success();
                k_msleep(3000);
                saved_index_joystick = 0;
            }
        }

                printk("\n");

        update_battery_display();

        if (seconds == 0) {
            display_not_success();
            break;
        }

		k_sleep(K_MSEC(100));
	}

    printk("Quadrature decoder sensor test\n");
    
    led_on_idx(rotary_idx, LEFT);

    while (true) {
        if (password_matched) {
            display_success();
            break;
        }

        if (!flag) {
            display_not_success();
            saved_index = 0;
            k_msleep(3000);
            flag = true;
            seconds = 120;
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

        // ë°°í??�°ë�?�??? ë??��?��?Š¤í??�Œë ˆ�?´ ì??��?�ë°�?´íŠ¸
        update_battery_display();

        if (seconds == 0) {
            display_not_success();
            break;
        }

        k_msleep(750);
    }

    return 0;
}