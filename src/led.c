#include "led.h"

const uint8_t led_patterns[10][8] = {
    {0b11111111, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b11111111}, // 0
    {0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b00010000}, // 1
    {0b11111111, 0b00000001, 0b00000001, 0b11111111, 0b10000000, 0b10000000, 0b11111111, 0b00000000}, // 2
    {0b11111111, 0b00000001, 0b00000001, 0b11111111, 0b00000001, 0b00000001, 0b11111111, 0b00000000}, // 3
    {0b10000001, 0b10000001, 0b10000001, 0b11111111, 0b00000001, 0b00000001, 0b00000001, 0b00000000}, // 4
    {0b11111111, 0b10000000, 0b10000000, 0b11111111, 0b00000001, 0b00000001, 0b11111111, 0b00000000}, // 5
    {0b11111111, 0b10000000, 0b10000000, 0b11111111, 0b10000001, 0b10000001, 0b11111111, 0b00000000}, // 6
    {0b11111111, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000}, // 7
    {0b11111111, 0b10000001, 0b10000001, 0b11111111, 0b10000001, 0b10000001, 0b11111111, 0b00000000}, // 8
    {0b11111111, 0b10000001, 0b10000001, 0b11111111, 0b00000001, 0b00000001, 0b11111111, 0b00000000}  // 9
};

const uint8_t password_success[8] = { // 4자리 비번이 맞을 때 나와야 하는 부분
    0b00000000, 
    0b01000010, 
    0b10100101, 
    0b00000000, 
    0b00000000, 
    0b01000010, 
    0b00111100, 
    0b00000000
};

const uint8_t password_unsuccess[8] = { // 4자리 비번이 안맞을 때 나와야 하는 부분
    0b00000000, 
    0b10100101, 
    0b01000010, 
    0b00000000, 
    0b00000000, 
    0b00111100, 
    0b01000010, 
    0b00000000
};

int led_init(void)
{
    if (!device_is_ready(led)) {
        printk("LED device %s is not ready\n", led->name);
        return -1;
    }
    return 0;
}

void led_off_all(void)
{
    for (int i = 0; i < MAX_LED_NUM; i++) {
        if (led_off(led, i) < 0) {
            printk("Failed to turn off LED %d\n", i);
        }
    }
}

void led_on_idx(int idx, bool right)
{
    if (idx < 0 || idx >= 10) {
        printk("Invalid index %d\n", idx);
        return;
    }
    display_pattern(led_patterns[idx], right);
}

void led_on_center(void)
{
    // led_off_all();
    led_set_brightness(led, 0, 0);

    for(int i = 38; i < 95; i+=16){
        led_on(led, i);
        led_on(led, i+1);
        led_on(led, i+2);
        led_on(led, i+3);
    }

    k_sleep(K_MSEC(100));
}

void led_on_right(void)
{
    // led_off_all();

    led_on(led, 26);
    led_on(led, 38); led_on(led, 39); led_on(led, 40); led_on(led, 41); led_on(led, 42); led_on(led, 43);
    led_on(led, 54); led_on(led, 55); led_on(led, 56); led_on(led, 57); led_on(led, 58); led_on(led, 59); led_on(led, 60); led_on(led, 61);
    led_on(led, 70); led_on(led, 71); led_on(led, 72); led_on(led, 73); led_on(led, 74); led_on(led, 75); led_on(led, 76); led_on(led, 77);
    led_on(led, 86); led_on(led, 87); led_on(led, 88); led_on(led, 89); led_on(led, 90); led_on(led, 91);
    led_on(led, 106);

    k_sleep(K_MSEC(100));
}

void led_on_left(void)
{
    // led_off_all();

    led_on(led, 21);
    led_on(led, 36); led_on(led, 37); led_on(led, 38); led_on(led, 39); led_on(led, 40); led_on(led, 41);
    led_on(led, 50); led_on(led, 51); led_on(led, 52); led_on(led, 53); led_on(led, 54); led_on(led, 55); led_on(led, 56); led_on(led, 57);
    led_on(led, 66); led_on(led, 67); led_on(led, 68); led_on(led, 69); led_on(led, 70); led_on(led, 71); led_on(led, 72); led_on(led, 73);
    led_on(led, 84); led_on(led, 85); led_on(led, 86); led_on(led, 87); led_on(led, 88); led_on(led, 89);
    led_on(led, 101);

    k_sleep(K_MSEC(100));
}

void led_on_up(void)
{
    // led_off_all();

    led_on(led, 23); led_on(led, 24);
    led_on(led, 37); led_on(led, 38); led_on(led, 39); led_on(led, 40); led_on(led, 41); led_on(led, 42);
    led_on(led, 52); led_on(led, 53); led_on(led, 54); led_on(led, 55); led_on(led, 56); led_on(led, 57); led_on(led, 58); led_on(led, 59);
    led_on(led, 70); led_on(led, 71); led_on(led, 72); led_on(led, 73);
    led_on(led, 86); led_on(led, 87); led_on(led, 88); led_on(led, 89);

    k_sleep(K_MSEC(100));
}

void led_on_down(void)
{
    // led_off_all();

    led_on(led, 38); led_on(led, 39); led_on(led, 40); led_on(led, 41);
    led_on(led, 54); led_on(led, 55); led_on(led, 56); led_on(led, 57);
    led_on(led, 68); led_on(led, 69); led_on(led, 70); led_on(led, 71); led_on(led, 72); led_on(led, 73); led_on(led, 74); led_on(led, 75);
    led_on(led, 85); led_on(led, 86); led_on(led, 87); led_on(led, 88); led_on(led, 89); led_on(led, 90);
    led_on(led, 103); led_on(led, 104);

    k_sleep(K_MSEC(100));

}

void display_pattern(const uint8_t pattern[8], bool right)
{
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int led_index = row * 16 + col;  // Default for left 8x8 part
            if (right) {
                led_index += 8; // Shift to the right 8x8 part
            }
            if (led_index >= 0 && led_index < MAX_LED_NUM) {
                if (pattern[row] & (1 << (7 - col))) {
                    if (led_on(led, led_index) < 0) {
                        printk("Failed to turn on LED %d\n", led_index);
                    }
                } else {
                    if (led_off(led, led_index) < 0) {
                        printk("Failed to turn off LED %d\n", led_index);
                    }
                }
            } else {
                printk("LED index %d out of range\n", led_index);
            }
        }
    }
}

void display_success_left(void)
{
    display_pattern(password_success, LEFT);
    display_pattern(password_success, RIGHT);
}

void display_success_right(void)
{
    display_pattern(password_success, RIGHT);
}

void display_not_success_left(void)
{
    display_pattern(password_unsuccess, LEFT);
    display_pattern(password_unsuccess, RIGHT);
}

void display_not_success_right(void)
{
    display_pattern(password_unsuccess, RIGHT);
}
