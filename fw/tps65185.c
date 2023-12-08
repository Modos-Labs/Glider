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
#include "tps65185.h"

#define TPS_I2C i2c0
#define TPS_I2C_ADDR 0x68
//#define TPS_I2C_ADDR 0x34

#define TPS_EN_PIN 21

void tps_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2];
    int result;
    buf[0] = reg;
    buf[1] = val;
    result = i2c_write_blocking(TPS_I2C, TPS_I2C_ADDR, buf, 2, false);
    if (result != 2) {
        fatal("Failed writing data to TPS");
    }
}

uint8_t tps_read(uint8_t reg) {
    int result;
    uint8_t buf[1];
    buf[0] = reg;
    result = i2c_write_blocking(TPS_I2C, TPS_I2C_ADDR, buf, 1, true);
    if (result != 1) {
        fatal("Failed writing data to TPS");
    }
    result = i2c_read_blocking(TPS_I2C, TPS_I2C_ADDR, buf, 1, false);
    if (result != 1) {
        fatal("Failed reading data from TPS");
    }
    return buf[0];
}

void tps_set_vcom(uint16_t vcom) {
    tps_write(0x04, (vcom >> 16));
    tps_write(0x03, vcom & 0xff);
}

void tps_enable(bool en) {
    gpio_put(TPS_EN_PIN, en);
}

void tps_init(void) {
    gpio_init(TPS_EN_PIN);
    gpio_put(TPS_EN_PIN, 1);
    gpio_set_dir(TPS_EN_PIN, GPIO_OUT);
    for (int i = 0; i < 100; i++) {
        sleep_ms(5);
        uint8_t val = tps_read(0x0f);
        printf("Val: %08x\n", val);
    }
    sleep_ms(1000);
    tps_set_vcom(212); // -1V
    printf("VADJ: %08x\n", tps_read(0x02));
    printf("UPSEQ0: %08x\n", tps_read(0x09));
    printf("UPSEQ1: %08x\n", tps_read(0x0a));
    printf("INT1: %08x\n", tps_read(0x07));
    printf("INT2: %08x\n", tps_read(0x08));
}