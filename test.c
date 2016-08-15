#include <qm_soc_regs.h>
#include <qm_gpio.h>
#include <qm_pinmux.h>
#include <clk.h>

#include <stdbool.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define LED_BOTTOM_LEFT     6
#define LED_BOTTOM_MIDDLE   0
#define LED_BOTTOM_RIGHT    1
#define LED_EYE_LEFT        18
#define LED_EYE_RIGHT       2
#define LED_ENABLE          9

#define BTN_L_UP            14
#define BTN_L_DOWN          17
#define BTN_L_LEFT          16
#define BTN_L_RIGHT         15
#define BTN_R_UP            11
#define BTN_R_DOWN          7
#define BTN_R_LEFT          10
#define BTN_R_RIGHT         8

const int led_gpios[] =
{
    LED_BOTTOM_MIDDLE,
    LED_BOTTOM_RIGHT,
    LED_EYE_RIGHT,
    LED_BOTTOM_LEFT,
    LED_EYE_LEFT
};

#define NUM_LEDS ARRAY_SIZE(led_gpios)

const int button_gpios[] =
{
    BTN_L_UP,
    BTN_L_DOWN,
    BTN_L_LEFT,
    BTN_L_RIGHT,
    BTN_R_UP,
    BTN_R_DOWN,
    BTN_R_LEFT,
    BTN_R_RIGHT
};

#define NUM_BUTTONS ARRAY_SIZE(button_gpios)

int num_flashes = 3;
int sleep_time = 15;
int led_on_time = 2000;

// Initialize with anything other than zero.
uint32_t lfsr_state = 1;

uint32_t old_buttons = ~0;

// Based on RANDOM from:
// https://www.schneier.com/academic/archives/1994/09/pseudo-random_sequen.html
int lfsr()
{
    lfsr_state = ((((lfsr_state >> 31)  // Shift the 32nd bit to the first bit
             ^ (lfsr_state >> 6)        // XOR it with the seventh bit
             ^ (lfsr_state >> 4)        // XOR it with the fifth bit
             ^ (lfsr_state >> 2)        // XOR it with the third bit
             ^ (lfsr_state >> 1)        // XOR it with the second bit
             ^ lfsr_state)              // and XOR it with the first bit.
             & 0x0000001)               // Strip all the other bits off and
             <<31)                      // move it back to the 32nd bit.
             | (lfsr_state >> 1);       // Or with the register shifted right.
    return lfsr_state;
}

void special1()
{
    qm_gpio_set_pin(QM_GPIO_0, LED_EYE_LEFT);
    qm_gpio_set_pin(QM_GPIO_0, LED_EYE_RIGHT);

    // Eyes up.
    for (int i = 0; i < 120; i++)
    {
        qm_gpio_set_pin(QM_GPIO_0, LED_ENABLE);
        clk_sys_udelay(i * 69);
        qm_gpio_clear_pin(QM_GPIO_0, LED_ENABLE);
        clk_sys_udelay((120 - i) * 69);
    }

    // Eyes down.
    for (int i = 0; i < 120; i++)
    {
        qm_gpio_set_pin(QM_GPIO_0, LED_ENABLE);
        clk_sys_udelay((120 - i) * 69);
        qm_gpio_clear_pin(QM_GPIO_0, LED_ENABLE);
        clk_sys_udelay(i * 69);
    }

    qm_gpio_clear_pin(QM_GPIO_0, LED_EYE_LEFT);
    qm_gpio_clear_pin(QM_GPIO_0, LED_EYE_RIGHT);
}

void special2()
{
    qm_gpio_set_pin(QM_GPIO_0, LED_ENABLE);

    for (int i = 0; i < 5; i++)
    {
        qm_gpio_set_pin(QM_GPIO_0, LED_EYE_RIGHT);
        clk_sys_udelay(20000);
        qm_gpio_clear_pin(QM_GPIO_0, LED_EYE_RIGHT);
        clk_sys_udelay(30000);
        qm_gpio_set_pin(QM_GPIO_0, LED_EYE_LEFT);
        clk_sys_udelay(20000);
        qm_gpio_clear_pin(QM_GPIO_0, LED_EYE_LEFT);
        clk_sys_udelay(30000);
    }

    qm_gpio_clear_pin(QM_GPIO_0, LED_EYE_LEFT);
    qm_gpio_clear_pin(QM_GPIO_0, LED_EYE_RIGHT);
    qm_gpio_clear_pin(QM_GPIO_0, LED_ENABLE);
}

bool poll_buttons()
{
    uint32_t cur_buttons = ~QM_GPIO[0]->gpio_ext_porta;
    uint32_t new_buttons = cur_buttons & ~old_buttons;
    old_buttons = cur_buttons;

    if (cur_buttons & BIT(BTN_R_UP))
    {
        if (new_buttons & BIT(BTN_L_UP))
        {
            led_on_time = 500;
        } else if (new_buttons & BIT(BTN_L_RIGHT))
        {
            led_on_time = 1000;
        } else if (new_buttons & BIT(BTN_L_DOWN))
        {
            led_on_time = 2000;
        } else if (new_buttons & BIT(BTN_L_LEFT))
        {
            led_on_time = 5000;
        } else if (new_buttons & BIT(BTN_R_RIGHT))
        {
            led_on_time = 10000;
        } else if (new_buttons & BIT(BTN_R_DOWN))
        {
            special1();
        } else if (new_buttons & BIT(BTN_R_LEFT))
        {
            special2();
        }
    } else {
        if (new_buttons & BIT(BTN_L_UP))
        {
            sleep_time = 2;
        } else if (new_buttons & BIT(BTN_L_RIGHT))
        {
            sleep_time = 5;
        } else if (new_buttons & BIT(BTN_L_DOWN))
        {
            sleep_time = 10;
        } else if (new_buttons & BIT(BTN_L_LEFT))
        {
            sleep_time = 15;
        }

        if (new_buttons & BIT(BTN_R_RIGHT))
        {
            num_flashes = 1;
        } else if (new_buttons & BIT(BTN_R_DOWN))
        {
            num_flashes = 3;
        } else if (new_buttons & BIT(BTN_R_LEFT))
        {
            num_flashes = 5;
        }
    }

    return new_buttons != 0;
}

void fast()
{
    clk_sys_set_mode(CLK_SYS_HYB_OSC_4MHZ, CLK_SYS_DIV_1);
}

void slow()
{
    clk_sys_set_mode(CLK_SYS_HYB_OSC_4MHZ, CLK_SYS_DIV_4);
}

int main()
{
    fast();

    qm_gpio_port_config_t cfg =
    {
        .direction = BIT(LED_BOTTOM_LEFT) | BIT(LED_BOTTOM_MIDDLE) | BIT(LED_BOTTOM_RIGHT) |
                     BIT(LED_EYE_LEFT) | BIT(LED_EYE_RIGHT) | BIT(LED_ENABLE)
    };
    qm_gpio_set_config(QM_GPIO_0, &cfg);

    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        qm_pmux_pullup_en(button_gpios[i], true);
        qm_pmux_input_en(button_gpios[i], true);
    }

    while (1)
    {
        int led = lfsr() % NUM_LEDS;

        qm_gpio_set_pin(QM_GPIO_0, led_gpios[led]);

        for (int i = 0; i < num_flashes; i++)
        {
            qm_gpio_set_pin(QM_GPIO_0, LED_ENABLE);
            clk_sys_udelay(led_on_time);
            qm_gpio_clear_pin(QM_GPIO_0, LED_ENABLE);

            clk_sys_udelay(100000);
        }

        qm_gpio_clear_pin(QM_GPIO_0, led_gpios[led]);

        for (int i = 0; i < sleep_time; i++)
        {
            if (poll_buttons())
            {
                break;
            }
            slow();
            clk_sys_udelay(100000);
            fast();
        }
    }
}