//
// Copyright 2024 Wenting Zhang <zephray@outlook.com>
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
#include "button.h"

#define BTN1	6
#define BTN2	5

#define BTNCNT	2

#define SHORT_PRESS_THRESHOLD 	20
#define LONG_PRESS_THRESHOLD	500
#define RELEASE_THRESHOLD		20

void button_init() {
    gpio_init(BTN1);
    gpio_set_dir(BTN1, GPIO_IN);
    gpio_pull_up(BTN1);

    gpio_init(BTN2);
    gpio_set_dir(BTN2, GPIO_IN);
    gpio_pull_up(BTN2);
}

// Scan a single key, ID is the key number from 0, gpio is pin number
// Should be called at ~1ms interval
// Return value:
// 0 when nothing pressed
// 1 when short pressed
// 2 when long pressed
static uint32_t button_scan_single(int id, int gpio) {
	static int timecntr[BTNCNT] = {0};
	static enum {
		STATE_IDLE,
		STATE_FIRST_PRESSED,
		STATE_HOLDING,
		STATE_RELEASED
	} state[BTNCNT] = {0};
	uint32_t retval = 0;

	switch (state[id]) {
	case STATE_IDLE:
		if (gpio_get(gpio) == 0) {
			state[id] = STATE_FIRST_PRESSED;
			timecntr[id] = 0;
		}
		break;
	case STATE_FIRST_PRESSED:
		timecntr[id]++;
		if (gpio_get(gpio) == 0) {
			// Still pressing
			if (timecntr[id] >= SHORT_PRESS_THRESHOLD) {
				state[id] = STATE_HOLDING;
				timecntr[id] = 0;
			}
		}
		else {
			// No longer pressed
			state[id] = STATE_IDLE;
		}
		break;
	case STATE_HOLDING:
		timecntr[id]++;
		if (gpio_get(gpio) == 0) {
			// Still holding
			if (timecntr[id] == LONG_PRESS_THRESHOLD) {
				// Exactly when reaching the threshold, return the key press
				// User gets feedback as soon as the threshold is reached
				retval = 2;
			}
		}
		else {
			// No longer holding
			if (timecntr[id] < LONG_PRESS_THRESHOLD) {
				// Is a short press, signal
				retval = 1;
			}
			timecntr[id] = 0;
			state[id] = STATE_RELEASED;
		}
		break;
	case STATE_RELEASED:
		if (gpio_get(gpio) == 0) {
			// Pressed, reset time counter
			timecntr[id] = 0;
		}
		else {
			// Keep counting time
			// Only treat key as released when reaching threshold
			timecntr[id]++;
			if (timecntr[id] >= RELEASE_THRESHOLD) {
				state[id] = STATE_IDLE;
			}
		}
		break;
	}

	return retval;
}

uint32_t button_scan() {
	uint32_t btn1 = button_scan_single(0, BTN1);
	uint32_t btn2 = button_scan_single(1, BTN2);
	uint32_t retval = (btn1 & 0x3) | ((btn2 & 0x3) << 2);
	return retval;
}
