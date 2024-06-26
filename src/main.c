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
#include "cts.h"

#include <zephyr/types.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/services/ias.h>

// [BLE Part]
// Custom Service Variables
#define BT_UUID_CUSTOM_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define BT_UUID_CUSTOM_MESSAGE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef5)

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_message_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_MESSAGE_VAL);

#define CUSTOM_MESSAGE_MAX_LEN 50

static uint8_t custom_message_value[CUSTOM_MESSAGE_MAX_LEN + 1] = "your safe is secured."; //when connect device with bluetooth

static ssize_t read_custom_message(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
    const char *value = attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

// Custom Service Declaration
BT_GATT_SERVICE_DEFINE(custom_svc,
                       BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
                       BT_GATT_CHARACTERISTIC(&custom_message_uuid.uuid,
                                              BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ,
                                              read_custom_message, NULL, custom_message_value));

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
    printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
    .att_mtu_updated = mtu_updated};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        printk("Connection failed (err 0x%02x)\n", err);
    }
    else
    {
        printk("Connected\n");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static void bt_ready(void)
{
    int err;

    printk("Bluetooth initialized\n");

    cts_init();

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

void start_bluetooth(void)
{
    int err;

    err = bt_enable(NULL);
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    bt_ready();

    bt_gatt_cb_register(&gatt_callbacks);

    printk("Bluetooth initialized\n");
}

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

#define MAX_SAVED_NUMBERS 4 //MAX number of password.
static bool sw_led_flag = false; // can save number in encoder. 
static int saved_numbers[MAX_SAVED_NUMBERS] = { -1, -1, -1, -1 }; // when click the encoder, the number will save. 
static int saved_index = 0; //count the number that saved by rotary
static int password[MAX_SAVED_NUMBERS] = {1, 2, 3, 4}; // password of locker, (it can change by user)
static int password_matched = -1; //check if password is matched or not
int flag_password_moved = false;
int time_out = false; //break when time_out get true
int success = false; //when password success it will quit program.

static struct gpio_callback sw_cb_data;

extern const uint8_t led_patterns[10][8];

// [Joystick Part]
static int saved_number_joystick[MAX_SAVED_NUMBERS] = { -1, -1, -1, -1 };
static int password_joystick[MAX_SAVED_NUMBERS] = {1, 2, 3, 4}; // Password of joystick
static int saved_index_joystick = 0;
int flag_joystick = false;
int flag_joystick_moved = false;

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

int32_t preX = 0 , perY = 0;
static const int ADC_MAX = 1023;
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

void sw_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) //encoder click 하는 부분
{
    printk("SW pressed, displaying number %d on the right matrix\n", rotary_idx);
    sw_led_flag = true;
    flag_password_moved = true;

    // Save the current number
    saved_numbers[saved_index++] = rotary_idx;
  
    // Print saved numbers
    if (saved_index == MAX_SAVED_NUMBERS) {  // when saved index has 4 number.
        printk("complete\n");
        printk("Saved numbers: ");
        for (int i = 0; i < MAX_SAVED_NUMBERS; i++) { //number that user save (just print even is false)
            printk("%d ", saved_numbers[i]);
        }
        printk("\n");
        
        if (compare_arrays(saved_numbers, password, MAX_SAVED_NUMBERS)) { //compare if password is correct or wrong
            printk("Password matched!\n");
            password_matched = true;
            strncpy(custom_message_value, "password success", CUSTOM_MESSAGE_MAX_LEN); //send "password success" to bluetooth
        } else {
            printk("Password not matched!\n");
            password_matched = false;
            strncpy(custom_message_value, "password fail", CUSTOM_MESSAGE_MAX_LEN);
        }
    }
}

void check_password_matching(void) {
    if (password_matched) { //if password_matched is true display smile face to LED matrix
        display_success(); 
        success = true; //progroam quit
    } else {
        display_not_success();
        saved_index = 0; // initialize saved_index to 0 
        k_msleep(3000);
        rotary_idx = 0; // reset led matrix to 0 when password fail
        display_pattern(led_patterns[rotary_idx], RIGHT); // LED matrix to 0 - right
        display_pattern(led_patterns[rotary_idx], LEFT);  // LED matrix to 0 - left
    }
}

void display_rotary_led(int32_t rotary_val)
{
    if (rotary_val == 0) { //make led matrix increase or decrease when rotate the encoder more or less than 17 degree
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
static int seconds = 121; //
int seconds_count = 0;

//battery gage per sec
void update_battery_display(int stage)
{
    // Battery Display Level
    uint8_t level = 0;

    // every 12 seconds battery level get change
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

    // show level of battery
    display_level(level);

    // time decrease
    if (stage == 1 && seconds_count == 10) {
        seconds--;
        seconds_count = 0;
    }

    if (stage == 2) {
        seconds--;
    }

    if (seconds < 0) {
        time_out = true;
        display_not_success();
        strncpy(custom_message_value, "time out", CUSTOM_MESSAGE_MAX_LEN); // send "time out" to app
    }

    seconds_count++;
}

int bluetooth = true; 

int main(void)
{
    if (bluetooth) { //connect to bluetooth
        start_bluetooth();
        bluetooth = false; //didn't need to connect again
    }

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

    // I2C Matrix initialize
    if (led_init() < 0) {
        printk("LED init failed\n");
        return 0;
    }
  
    // battery display initialize
    if (batterydisplay_init() < 0) {
        printk("Battery display init failed\n");
        return 0;
    }

    // Stage 1. Password by Joystick
    while (1) {
        //printk("ADC reading[%u]: ", count++);

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

        //printk("Joy X: %" PRIu32 ", ", nowX);
        //printk("Joy Y: %" PRIu32 ", ", nowY);

        if (flag_joystick_moved == true) {
            strncpy(custom_message_value, "Your safe is being opened(joystick)", CUSTOM_MESSAGE_MAX_LEN); // 
            update_battery_display(1);
            printk("seconds: %d\n", seconds);
        }

        if (nowX >= 65500 || nowY >= 65500){
            printk("Out of Range\n");
            k_sleep(K_MSEC(100));
            continue;
        }

        bool checkFlag = isChange();
        if(!checkFlag){
            //printk("No Change\n");
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
            flag_joystick_moved = true;
            printk("Left");
        } else if (nowX > AXIS_DEVIATION && nowY == ADC_MAX) {
            led_on_right();
            if (flag_joystick) {
                saved_number_joystick[saved_index_joystick++] = 2;
            }
            flag_joystick = false;
            flag_joystick_moved = true;
            printk("Right");
        } else if (nowY > AXIS_DEVIATION && nowX == ADC_MAX){
            led_on_up();

            if (flag_joystick) {
                saved_number_joystick[saved_index_joystick++] = 1;
            }
            flag_joystick = false;
            flag_joystick_moved = true;
            printk("Up");
        } else if (nowY < AXIS_DEVIATION && nowX == ADC_MAX){
            led_on_down();

            if (flag_joystick) {
                saved_number_joystick[saved_index_joystick++] = 3;
            }
            flag_joystick = false;
            flag_joystick_moved = true;
            printk("Down");
        }
        
        if (saved_index_joystick == MAX_SAVED_NUMBERS) {
            if (compare_arrays(saved_number_joystick, password_joystick, MAX_SAVED_NUMBERS)) {
                led_off_all();
                display_success();
                k_msleep(3000);
                led_off_all();
                strncpy(custom_message_value, "Joystick success", CUSTOM_MESSAGE_MAX_LEN);
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

        if (time_out) {
            display_not_success();
            strncpy(custom_message_value, "someone failed to unlock your safe", CUSTOM_MESSAGE_MAX_LEN);
            break;
        }

        k_sleep(K_MSEC(100));
    }

    printk("Quadrature decoder sensor test\n");
    
    led_on_idx(rotary_idx, LEFT);

    // Stage 2. Password by Rotary Encoder
    while (true) {
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

        if (!sw_led_flag) { // Display current rotary pattern on the left side
            display_rotary_led(val.val1);
        } else { // Display selected number on the right side
            display_pattern(led_patterns[rotary_idx], RIGHT);
            sw_led_flag = false; // Reset flag to allow continuous update
        }

        printk("current value: %d\n", rotary_idx);

        if (flag_password_moved == true) {
            strncpy(custom_message_value, "Your safe is being opened(password)", CUSTOM_MESSAGE_MAX_LEN);
        }

        // update battery level
        update_battery_display(2);
        printk("seconds: %d\n", seconds);

        if (saved_index == 4) {
            check_password_matching();
        }

        // termination condition
        if (time_out) {
            display_not_success();
            strncpy(custom_message_value, "someone failed to unlock your safe", CUSTOM_MESSAGE_MAX_LEN);
            break;
        }

        if (success) {
            break;
        }

        k_msleep(1000);
    }

    strncpy(custom_message_value, "Your safe is opened!", CUSTOM_MESSAGE_MAX_LEN);

    return 0;
}
