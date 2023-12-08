//
// Copyright 2022 Wenting Zhang <zephray@outlook.com>
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
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "max17135.h"

#define MAX_I2C i2c0
#define MAX_I2C_ADDR 0x48

void max_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2];
    int result;
    buf[0] = reg;
    buf[1] = val;
    result = i2c_write_blocking(MAX_I2C, MAX_I2C_ADDR, buf, 2, false);
    if (result != 2) {
        fatal("Failed writing data to MAX");
    }
}

uint8_t max_read(uint8_t reg) {
    int result;
    uint8_t buf[1];
    buf[0] = reg;
    result = i2c_write_blocking(MAX_I2C, MAX_I2C_ADDR, buf, 1, true);
    if (result != 1) {
        fatal("Failed writing data to MAX");
    }
    result = i2c_read_blocking(MAX_I2C, MAX_I2C_ADDR, buf, 1, false);
    if (result != 1) {
        fatal("Failed reading data from MAX");
    }
    return buf[0];
}

void max_set_vcom(uint16_t vcom) {
    max_write(0x04, (vcom >> 16));
    max_write(0x03, vcom & 0xff);
}

void max_enable(bool en) {
    // TODO
}

void max_init(void) {
    //max_set_vcom(100); // -1V
    printf("Product Revision: %08x\n", max_read(0x06));
    printf("Product ID: %08x\n", max_read(0x07));
    max_write(0x10, 0x80);
    max_write(0x09, 0x01); // Startup
    for (int i = 0; i < 20; i++) {
        sleep_ms(10);
        uint8_t val = max_read(0x05);
        printf("Status: %08x\n", val);
    }
    printf("Fault: %08x\n", max_read(0x0a));
}