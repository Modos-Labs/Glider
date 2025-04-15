//
// Grimoire
// Copyright 2025 Wenting Zhang
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
#include "platform.h"
#include "board.h"
#include "app.h"

void power_on(void) {

}

void power_off(void) {

}

void power_on_epd(void) {
    gpio_put(VCOM_MEN, 1); // Disable
    gpio_put(VCOM_EN, 1); // Disable
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    gpio_put(EPD_PWREN, 1);
    sleep_ms(100);
    gpio_put(EPD_POSEN, 1);
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);
    sleep_ms(10);
    gpio_put(VCOM_EN, 0); // Enable
}

void power_off_epd(void) {
    gpio_put(VCOM_EN, 1); // Disable
	sleep_ms(10);
	gpio_put(EPD_POSEN, 0);
	HAL_DAC_Stop(&hdac1, DAC_CHANNEL_2);
	sleep_ms(10);
    gpio_put(EPD_PWREN, 0);
    HAL_DAC_Stop(&hdac1, DAC_CHANNEL_1);
}

void power_set_vcom(float vcom) {
    // DAC 000 = -2.667V
    // DAC 19A = -2.404V
    // DAC E80 = -0.226V
    // DAC FF0 = -0.011V
    // V = 0.000651 * set - 2.667
    // set = (V + 2.676) / 0.000651
    float setpt = (vcom + 2.676f) / 0.00066f;
    int setpt_i = (int)roundf(setpt);
    if (setpt_i < 0)
        setpt_i = 0;
    if (setpt_i > 4095)
        setpt_i = 4095;

    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, setpt_i);
}

void power_set_vgh(float vgh) {
    // DAC 000 26.87V
    // DAC 0F2 26.19V
    // DAC 77E 20.95V
    // DAC FF0 12.26V
	// Valid range: 22V - 27V
    // set = (26.945 - V) / 0.03126
    float setpt = (26.945f - vgh) / 0.003126f;
    int setpt_i = (int)roundf(setpt);
    if (setpt_i < 0)
        setpt_i = 0;
    if (setpt_i > 4095)
        setpt_i = 4095;

    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, setpt_i);
}

void power_on_fl(void) {
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
}

void power_off_fl(void) {
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
}

void power_set_fl_brightness(uint8_t val) {
	TIM1->CCR3 = 255 - val;
}

portTASK_FUNCTION(power_monitor_task, pvParameters) {
    power_on_epd();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Nothing for now
    }
}
