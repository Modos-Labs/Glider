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

    ADV7611_I2C_ADDR,   0x01, 0x06, // Prim_mode = 110b HDMI-GR
    ADV7611_I2C_ADDR,   0x00, 0x16, // VID-STD: UXGA (for default clock)
    ADV7611_I2C_ADDR,   0x02, 0xf2, // F8 = YUV, F2 = RGB
    ADV7611_I2C_ADDR,   0x03, 0x40, // 40 = 24bit 444 SDR
    ADV7611_I2C_ADDR,   0x04, 0x46, // P[23:16] V/R, P[15:8] Y/G, P[7:0] U/CrCb/B CLK=24MHz
    ADV7611_I2C_ADDR,   0x05, 0x28, // Do not insert AV codes
    ADV7611_I2C_ADDR,   0x06, 0xa6, // VS OUT SEL, F/VS/HS/LLC POL
    ADV7611_I2C_ADDR,   0x0b, 0x44,
    ADV7611_I2C_ADDR,   0x0C, 0x42,
    ADV7611_I2C_ADDR,   0x15, 0x80,
    ADV7611_I2C_ADDR,   0x19, 0x8a, // Enable LLC DLL
    ADV7611_I2C_ADDR,   0x33, 0x40,
    ADV7611_I2C_ADDR,   0x14, 0x7f,
    CP_I2C_ADDR,        0xba, 0x00, // Disable free run
    //HDMI_I2C_ADDR,      0xbf, 0x01, // Bypass CP
    CP_I2C_ADDR,        0x6c, 0x00, // ADI required setting
    KSV_I2C_ADDR,       0x40, 0x81, // DSP_Ctrl4 :00/01 : YUV or RGB; 10 : RAW8; 11 : RAW10
    HDMI_I2C_ADDR,      0x9b, 0x03, // ADI required setting
    HDMI_I2C_ADDR,      0xc1, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xc2, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xc3, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xc4, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xc5, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xc6, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xc7, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xc8, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xc9, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xca, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xcb, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0xcc, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0x00, 0x00, // Set HDMI input Port A
    HDMI_I2C_ADDR,      0x83, 0xfe, // Termination for Port A
    HDMI_I2C_ADDR,      0x6f, 0x08, // ADI required setting
    HDMI_I2C_ADDR,      0x85, 0x1f, // ADI required setting
    HDMI_I2C_ADDR,      0x87, 0x70, // ADI required setting
    HDMI_I2C_ADDR,      0x8d, 0x04, // LFG
    HDMI_I2C_ADDR,      0x8e, 0x1e, // HFG
    HDMI_I2C_ADDR,      0x1a, 0x8a, // unmute audio
    HDMI_I2C_ADDR,      0x57, 0xda, // ADI required setting
    HDMI_I2C_ADDR,      0x58, 0x01, // ADI required setting
    HDMI_I2C_ADDR,      0x03, 0x98, // Set DIS_I2C_ZERO_COMPR 0x03[7]=1
    HDMI_I2C_ADDR,      0x4c, 0x44, // Set NEW_VS_PARAM 0x44[2]=1
    HDMI_I2C_ADDR,      0x75, 0x10,
};

static const uint8_t adv7611_init_2[] = {
    KSV_I2C_ADDR,       0x74, 0x01, // Enable the Internal EDID for Ports
    ADV7611_I2C_ADDR,   0x20, 0xf0, // HPD high
    HDMI_I2C_ADDR,      0x6c, 0xa2  // disable manual HPA
};

static void adv7611_send_init_seq(const uint8_t *seq, int entries) {
    uint8_t addr;
    uint8_t buf[2];
    int result;
    for (int i = 0; i < entries; i++) {
        addr = seq[i*3];
        buf[0] = seq[i*3 + 1];
        buf[1] = seq[i*3 + 2];
        result = pal_i2c_write_payload(ADV7611_I2C, addr, buf, 2);
        if (result != 0) {
            syslog_printf("Failed writing data to ADV7611\n");
        }
    }
}

static void adv7611_load_edid(uint8_t *edid) {
    uint8_t buf[2];
    int result;
    for (int i = 0; i < 128; i++) {
        buf[0] = i;
        buf[1] = edid[i];
        result = pal_i2c_write_payload(ADV7611_I2C, EDID_I2C_ADDR, buf, 2);
        if (result != 0) {
            syslog_printf("Failed writing data to ADV7611\n");
        }
    }
}

uint8_t adv7611_read_reg(uint8_t addr, uint8_t reg) {
    uint8_t val;
    int result = pal_i2c_read_reg(ADV7611_I2C, addr, reg, &val);
    if (result != 0) {
        syslog_printf("Failed reading data from ADV7611\n");
    }
    return val;
}

void adv7611_early_init() {
    // Initialize IO, reset ADV7611 and allocate I2C addresses
    // So it won't conflict with other ICs
    gpio_put(DEC_RST, 1);
    sleep_ms(10);
    gpio_put(DEC_RST, 0);
    sleep_ms(10);
    gpio_put(DEC_RST, 1);
}

void adv7611_init() {
    adv7611_send_init_seq(adv7611_init_0, sizeof(adv7611_init_0) / 3);
    adv7611_send_init_seq(adv7611_init_1, sizeof(adv7611_init_1) / 3);

    uint8_t *edid = edid_get_raw();
    adv7611_load_edid(edid);

    adv7611_send_init_seq(adv7611_init_2, sizeof(adv7611_init_2) / 3);

    syslog_printf("ADV7611 initialization done\n");
}
