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
#include "ui.h"

static QueueHandle_t btn_queue;

typedef enum {
    BTN1_SHORT_PRESSED,
    BTN1_LONG_PRESSED,
    BTN2_SHORT_PRESSED,
    BTN2_LONG_PRESSED
} btn_event_t;

static int mode = 1;

const update_mode_t modes[] = {
    UM_FAST_MONO_NO_DITHER,
    UM_FAST_MONO_BAYER,
    UM_FAST_MONO_BLUE_NOISE,
    UM_FAST_GREY,
    UM_AUTO_LUT_NO_DITHER,
    UM_AUTO_LUT_ERROR_DIFFUSION,
};

static int mode_max = sizeof(modes) / sizeof(modes[0]);

void ui_init(void) {
    btn_queue = xQueueCreate(8, sizeof(btn_event_t));
}

portTASK_FUNCTION(ui_task, pvParameters) {
    while (1) {
        // Key press logic
        btn_event_t btn_event;
        BaseType_t result = xQueueReceive(btn_queue, &btn_event, portMAX_DELAY);
        if (result != pdTRUE)
            continue;
        if (btn_event == BTN1_SHORT_PRESSED) {
            // First key short press
            mode--;
            if (mode < 0) mode = mode_max - 1;
            caster_setmode(0, 0, config.hact, config.vact, modes[mode]);
        }
        else if (btn_event == BTN2_SHORT_PRESSED) {
            mode++;
            if (mode >= mode_max) mode = 0;
            caster_setmode(0, 0, config.hact, config.vact, modes[mode]);
        }
        else if ((btn_event == BTN1_LONG_PRESSED) || (btn_event == BTN2_LONG_PRESSED)) {
            // First key long press
            // Clear screen
            caster_redraw(0,0,2400,1800);
        }
    }
}

portTASK_FUNCTION(key_scan_task, pvParameters) {
    while (1) {
        uint32_t scan = button_scan();
        btn_event_t event;
        // No wait, if the ui task doesn't take it, the key press is lost
        if ((scan & BTN_MASK) == BTN_SHORT_PRESSED) {
            event = BTN1_SHORT_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        else if ((scan & BTN_MASK) == BTN_LONG_PRESSED) {
            event = BTN1_LONG_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        if (((scan >> BTN_SHIFT) & BTN_MASK) == BTN_SHORT_PRESSED) {
            event = BTN2_SHORT_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        else if (((scan >> BTN_SHIFT) & BTN_MASK) == BTN_LONG_PRESSED) {
            event = BTN2_LONG_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
