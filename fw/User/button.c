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
#include "button.h"

// Unit: 10ms
#define SHORT_PRESS_THRESHOLD 	2
#define LONG_PRESS_THRESHOLD	50
#define RELEASE_THRESHOLD		2

static int timecntr[BTNCNT] = {0};
static enum {
    STATE_IDLE,
    STATE_FIRST_PRESSED,
    STATE_HOLDING,
    STATE_RELEASED
} state[BTNCNT] = {0};

void button_init() {
    memset(timecntr, 0, sizeof(timecntr));
    memset(state, 0, sizeof(state));
}

// Scan a single key, ID is the key number from 0, gpio is pin number
// Should be called at ~10ms interval
// Return value:
// 0 when nothing pressed
// 1 when short pressed
// 2 when long pressed
static uint32_t button_scan_single(int id, bool pressed) {
    uint32_t retval = 0;

    switch (state[id]) {
    case STATE_IDLE:
        if (pressed) {
            state[id] = STATE_FIRST_PRESSED;
            timecntr[id] = 0;
        }
        break;
    case STATE_FIRST_PRESSED:
        timecntr[id]++;
        if (pressed) {
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
        if (pressed) {
            // Still holding
            if (timecntr[id] == LONG_PRESS_THRESHOLD) {
                // Exactly when reaching the threshold, return the key press
                // User gets feedback as soon as the threshold is reached
                retval = BTN_LONG_PRESSED;
            }
        }
        else {
            // No longer holding
            if (timecntr[id] < LONG_PRESS_THRESHOLD) {
                // Is a short press, signal
                retval = BTN_SHORT_PRESSED;
            }
            timecntr[id] = 0;
            state[id] = STATE_RELEASED;
        }
        break;
    case STATE_RELEASED:
        if (pressed) {
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
    uint32_t btn1 = button_scan_single(0, gpio_get(BTN1) == BTN_PRESSED_LEVEL);
    uint32_t btn2 = button_scan_single(1, gpio_get(BTN2) == BTN_PRESSED_LEVEL);
    uint32_t retval = (btn1 & 0x3) | ((btn2 & 0x3) << 2);
    return retval;
}
