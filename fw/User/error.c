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

#define ERR_LINE_MAX    50
static char line[ERR_LINE_MAX];

void warning(char *fmt, ...) {
    // SHOW WARNING MESSAGE
    va_list args;

    va_start(args, fmt);
    vsnprintf(line, ERR_LINE_MAX, fmt, args);
    va_end(args);

    // TODO:  Show warning on screen
    syslog_print(line);
}

void error(char *fmt, ...) {
    // DISPLAY ERROR MESSAGE AND SHOW RED LED
    gpio_put(LED_RED, 1);

    va_list args;

    va_start(args, fmt);
    vsnprintf(line, ERR_LINE_MAX, fmt, args);
    va_end(args);

    // TODO:  Show error on screen and stop further operation
    syslog_print(line);
}

void fatal(char *fmt, ...) {
    va_list args;

    taskENTER_CRITICAL();

    // TURN OFF ALL COMPONENTS AND SHOW RED LED
    gpio_put(LED_RED, 1);
    power_off_epd();
    fpga_reset();

    va_start(args, fmt);
    vsnprintf(line, ERR_LINE_MAX, fmt, args);
    va_end(args);

    syslog_print(line);

    // Increase priority of the shell so it becomes the only thing to run
    vTaskPrioritySet(&startup_task_handle, HIGHEST_PRIORITY);

    taskEXIT_CRITICAL();

    while (1) {
        sleep_ms(100);
    }
}