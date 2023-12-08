//
// Copyright 2023 Wenting Zhang <zephray@outlook.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "config.h"

#if defined(POWER_GPIO)
#define PWR_EN_GPIO     21
#define PWR_VCOM_GPIO   22

void power_init(void) {
    gpio_put(PWR_EN_GPIO, 0);
    gpio_init(PWR_EN_GPIO);
    gpio_set_dir(PWR_EN_GPIO, GPIO_OUT);
    gpio_set_function(PWR_VCOM_GPIO, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWR_VCOM_GPIO);
    // Set period to 256 cycles
    pwm_set_wrap(slice_num, 255);
    pwm_set_gpio_level(PWR_VCOM_GPIO, 127);
}

void power_enable(bool en) {
    gpio_put(PWR_EN_GPIO, en);
    uint slice_num = pwm_gpio_to_slice_num(PWR_VCOM_GPIO);
    pwm_set_enabled(slice_num, en);
}

// vcom is in mV
void power_set_vcom(int vcom) {
    // Adjustable range:
    // 0 = -3.27V
    // 255 = -0.70V
    int level = vcom / 10 + 324;
    uint8_t level_u8;
    if (level < 0)
        level_u8 = 0;
    else if (level > 255)
        level_u8 = 255;
    else
        level_u8 = level;
    pwm_set_gpio_level(PWR_VCOM_GPIO, level_u8);
}

#endif