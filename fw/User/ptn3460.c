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

static void ptn3460_write(uint8_t reg, uint8_t val) {
    int result = pal_i2c_write_reg(PTN3460_I2C, PTN3460_I2C_ADDR, reg, val);
    if (result != 0) {
        syslog_printf("Failed writing data to PTN3460\n");
    }
}

static uint8_t ptn3460_read(uint8_t reg) {
	uint8_t val;
	int result = pal_i2c_read_reg(PTN3460_I2C, PTN3460_I2C_ADDR, reg, &val);
	if (result != 0) {
		syslog_printf("Failed reading data from PTN3460\n");
	}
    return val;
}

static void ptn3460_select_edid_emulation(uint8_t id) {
    ptn3460_write(0x84, 0x01 | (id << 1));
}

void ptn3460_load_edid(uint8_t *edid) {
    int result = pal_i2c_write_longreg(PTN3460_I2C, PTN3460_I2C_ADDR,
        0x00, edid, 128);
    if (result != 0) {
        syslog_printf("Failed writing data to PTN3460\n");
    }
}

void ptn3460_early_init(void) {
    gpio_put(DP_PDN, 1);
}

void ptn3460_init(void) {
    // wait for HPD to become high
    int ticks = 0;
    while (gpio_get(DP_HPD) != true) {
        ticks ++;
        if (ticks > 500) {
            syslog_printf("PTN3460 boot timeout\n");
        }
        sleep_ms(1);
    }
    syslog_printf("PTN3460 up after %d ms\n", ticks);
    // Enable EDID emulation
    ptn3460_select_edid_emulation(0);
    ptn3460_load_edid(edid_get_raw());

    ptn3460_write(0x81, 0x29); // 18bpp, clock on odd bus, dual channel
    uint8_t rdval = ptn3460_read(0x81);
    syslog_printf("PTN3460 readback value %02x (expected %02x)\n", rdval, 0x29);
}

void ptn3460_set_aux_polarity(int reverse) {
    if (reverse)
        ptn3460_write(0x80, 0x02); // Enable AUX reverse
    else
        ptn3460_write(0x80, 0x00); // Disable AUX reverse
}

void ptn3460_powerdown(void) {
    gpio_put(DP_PDN, 0);
}
