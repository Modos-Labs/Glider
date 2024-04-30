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
#include "hardware/i2c.h"
#include "config.h"
#include "adv7611.h"
#include "edid.h"

#ifdef INPUT_ADV7611

#define ADV7611_INT_PIN     (1)
#define ADV7611_RST_PIN     (25)

#define ADV7611_I2C         (i2c1)

#define ADV7611_I2C_ADDR    (0x4C)

// Sub addresses, make sure they don't conflict with anything else!
#define CEC_I2C_ADDR        (0x40) // Default 0x80(0x40)
#define INFOFRAME_I2C_ADDR  (0x3E) // Default 0x7C(0x3E)
#define DPLL_I2C_ADDR       (0x26) // Default 0x26(0x4C)
#define KSV_I2C_ADDR        (0x32) // Default 0x32(0x64)
#define EDID_I2C_ADDR       (0x36) // Default 0x36(0x6C)
#define HDMI_I2C_ADDR       (0x34) // Default 0x34(0x68)
#define CP_I2C_ADDR         (0x23) // Default 0x22(0x44)

static const uint8_t adv7611_init_0[] = {
    ADV7611_I2C_ADDR,   0xf4, (CEC_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xf5, (INFOFRAME_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xf8, (DPLL_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xf9, (KSV_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xfa, (EDID_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xfb, (HDMI_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xfd, (CP_I2C_ADDR << 1),
};

static const uint8_t adv7611_init_1[] = {
    ADV7611_I2C_ADDR,   0xf4, (CEC_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xf5, (INFOFRAME_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xf8, (DPLL_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xf9, (KSV_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xfa, (EDID_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xfb, (HDMI_I2C_ADDR << 1),
    ADV7611_I2C_ADDR,   0xfd, (CP_I2C_ADDR << 1),

    ADV7611_I2C_ADDR,   0x01, 0x05, // Prim_mode = 110b HDMI-GR
    ADV7611_I2C_ADDR,   0x00, 0x13,
    ADV7611_I2C_ADDR,   0x02, 0xf2, // F8 = YUV, F2 = RGB
    ADV7611_I2C_ADDR,   0x03, 0x40, // 40 = 444
    ADV7611_I2C_ADDR,   0x04, 0x42, // P[23:16] V/R, P[15:8] Y/G, P[7:0] U/CrCb/B CLK=28.63636MHz
    ADV7611_I2C_ADDR,   0x05, 0x28,
    ADV7611_I2C_ADDR,   0x06, 0xa7,
    ADV7611_I2C_ADDR,   0x0b, 0x44,
    ADV7611_I2C_ADDR,   0x0C, 0x42,
    ADV7611_I2C_ADDR,   0x15, 0x80,
    ADV7611_I2C_ADDR,   0x19, 0x8a,
    ADV7611_I2C_ADDR,   0x33, 0x40,
    ADV7611_I2C_ADDR,   0x14, 0x3f,
    CP_I2C_ADDR,        0xba, 0x01,
    CP_I2C_ADDR,        0x7c, 0x01,
    KSV_I2C_ADDR,       0x40, 0x81, // DSP_Ctrl4 :00/01 : YUV or RGB; 10 : RAW8; 11 : RAW10
    HDMI_I2C_ADDR,      0x9b, 0x03, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc1, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc2, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc3, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc4, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc5, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc6, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc7, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc8, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xc9, 0x01, // ADI recommanded settin g
    HDMI_I2C_ADDR,      0xca, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xcb, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0xcc, 0x01, // ADI recommanded setting
    HDMI_I2C_ADDR,      0x00, 0x00, // Set HDMI input Port A
    HDMI_I2C_ADDR,      0x83, 0xfe, // terminator for Port A
    HDMI_I2C_ADDR,      0x6f, 0x08, // ADI recommended setting
    HDMI_I2C_ADDR,      0x85, 0x1f, // ADI recommended setting
    HDMI_I2C_ADDR,      0x87, 0x70, // ADI recommended setting
    HDMI_I2C_ADDR,      0x8d, 0x04, // LFG
    HDMI_I2C_ADDR,      0x8e, 0x1e, // HFG
    HDMI_I2C_ADDR,      0x1a, 0x8a, // unmute audio
    HDMI_I2C_ADDR,      0x57, 0xda, // ADI recommended setting
    HDMI_I2C_ADDR,      0x58, 0x01,
    HDMI_I2C_ADDR,      0x75, 0x10,
    HDMI_I2C_ADDR,      0x6c, 0xa3, // enable manual HPA
    ADV7611_I2C_ADDR,   0x20, 0x70, // HPD low
    KSV_I2C_ADDR,       0x74, 0x00, // disable internal EDID
};

static const uint8_t adv7611_init_2[] = {
    KSV_I2C_ADDR,       0x74, 0x01, // Enable the Internal EDID for Ports
    ADV7611_I2C_ADDR,   0x20, 0xf0, // HPD high
    HDMI_I2C_ADDR,      0x6c, 0xa2, // disable manual HPA
    ADV7611_I2C_ADDR,   0xf4, 0x00
};

static void adv7611_send_init_seq(const uint8_t *seq, int entries) {
    uint8_t addr;
    uint8_t buf[2];
    int result;
    for (int i = 0; i < entries; i++) {
        addr = seq[i*3];
        buf[0] = seq[i*3 + 1];
        buf[1] = seq[i*3 + 2];
        result = i2c_write_blocking(ADV7611_I2C, addr, buf, 2, false);
        if (result != 2) {
            fatal("Failed writing data to ADV7611\n");
        }
    }
}

static void adv7611_load_edid(uint8_t *edid) {
    uint8_t buf[2];
    int result;
    for (int i = 0; i < 128; i++) {
        buf[0] = i;
        buf[1] = edid[i];
        result = i2c_write_blocking(ADV7611_I2C, EDID_I2C_ADDR, buf, 2, false);
        if (result != 2) {
            fatal("Failed writing data to ADV7611\n");
        }
    }
}

void adv7611_early_init() {
    // Initialize IO, reset ADV7611 and allocate I2C addresses
    // So it won't conflict with other ICs

    gpio_init(ADV7611_RST_PIN);
    gpio_set_dir(ADV7611_RST_PIN, GPIO_OUT);
    gpio_init(ADV7611_INT_PIN);
    gpio_set_dir(ADV7611_INT_PIN, GPIO_IN);

    gpio_put(ADV7611_RST_PIN, 1);
    sleep_ms(100);
    gpio_put(ADV7611_RST_PIN, 0);
    sleep_ms(100);
    gpio_put(ADV7611_RST_PIN, 1);
    sleep_ms(100);
}

void adv7611_init() {
    adv7611_send_init_seq(adv7611_init_0, sizeof(adv7611_init_0) / 3);
    adv7611_send_init_seq(adv7611_init_1, sizeof(adv7611_init_1) / 3);

    uint8_t *edid = edid_get_raw();
    adv7611_load_edid(edid + 1);

    adv7611_send_init_seq(adv7611_init_2, sizeof(adv7611_init_2) / 3);

    printf("ADV7611 initialization done\n");
}

bool adv7611_is_valid(void) {
    return false; // TODO
}

#endif
