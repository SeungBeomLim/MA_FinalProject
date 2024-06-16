#include "batterydisplay.h"

static const struct gpio_dt_spec clk = GPIO_DT_SPEC_GET(DT_ALIAS(gpio_clk), gpios);
static const struct gpio_dt_spec dio = GPIO_DT_SPEC_GET(DT_ALIAS(gpio_dio), gpios);

static int8_t leveltab[11] = {0x00, 0x20, 0x40, 0x60, 0x70, 0x78, 0x7a, 0x7c, 0x7d, 0x7e, 0x7f}; // Level 0~10
static uint8_t cmd_dispctrl = DISPLAY_BRIGHTEST + BRIGHTNESS_LEVEL1;
static int setlevel = 0;

int batterydisplay_init(void)
{
    if (!device_is_ready(clk.port)) {
        printk("clk GPIO is not ready\n");
        return -1;
    }

    if (!device_is_ready(dio.port)) {
        printk("dio GPIO is not ready\n");
        return -1;
    }

    printk("batterydisplay_init success\n");

    return 0;
}

void set_brightness(int brightness)
{
    cmd_dispctrl = DISPLAY_BRIGHTEST + brightness;
}

void set_level(int level)
{
    setlevel = level;
}

void write_byte(int8_t wr_data)
{
    uint8_t data = wr_data;

    for (uint8_t i = 0; i < 8; i++) {
        gpio_pin_configure_dt(&clk, GPIO_OUTPUT);
        bit_delay();

        if (data & 0x01)
            gpio_pin_configure_dt(&dio, GPIO_INPUT);
        else
            gpio_pin_configure_dt(&dio, GPIO_OUTPUT);

        bit_delay();

        gpio_pin_configure_dt(&clk, GPIO_INPUT);
        bit_delay();

        data >>= 1;
    }

    gpio_pin_configure_dt(&clk, GPIO_OUTPUT);
    gpio_pin_configure_dt(&dio, GPIO_INPUT);
    bit_delay();

    gpio_pin_configure_dt(&clk, GPIO_INPUT);
    bit_delay();
    uint8_t ack = gpio_pin_get_dt(&dio);
    if (ack == 0) {
        gpio_pin_configure_dt(&dio, GPIO_OUTPUT);
    }

    bit_delay();
    gpio_pin_configure_dt(&clk, GPIO_OUTPUT);
    bit_delay();
}

void start(void)
{
    gpio_pin_configure_dt(&dio, GPIO_OUTPUT);
    bit_delay();
}

void stop(void)
{
    gpio_pin_configure_dt(&dio, GPIO_OUTPUT);
    bit_delay();

    gpio_pin_configure_dt(&clk, GPIO_INPUT);
    bit_delay();

    gpio_pin_configure_dt(&dio, GPIO_INPUT);
    bit_delay();
}

int display_level(uint8_t level)
{
    if (level > 10) {
        printk("Invalid level\n");
        return -1;
    }

    if (setlevel == level) {
        return 0;
    }

    set_level(level);
    printk("display_level: %d\n", level);

    start();
    write_byte(ADDR_FIXED);
    stop();

    start();
    write_byte(ADDR_CMD_00H);
    write_byte(leveltab[level]);
    stop();

    start();
    write_byte(cmd_dispctrl);
    stop();

    printk("display_level success\n");

    return 0;
}

void display_clear(void)
{
    display_level(0);
}

void bit_delay(void)
{
    k_sleep(K_TICKS(1));
}