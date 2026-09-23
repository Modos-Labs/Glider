//
// Glider
// Copyright 2024 Wenting Zhang
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
#ifdef GLIDER_HOST_TEST
#include <stdint.h>
#else
#include "platform.h"
#include "board.h"
#include "app.h"
#endif

#include "config.h"

#ifdef GLIDER_HOST_TEST
uint32_t board_get_uid(void);
#endif

// EDID 1.3 digital input, DFP 1.x compatible TMDS. The same EDID is also
// loaded into the DP bridge, where hosts tolerate this better than 0x85.
#define EDID_VID_IN_PARAM   (0x81)
#define EDID_DTD_PREFERRED_OFFSET 54
#define EDID_DESC_RANGE_OFFSET    90
#define EDID_DESC_DUMMY_OFFSET    108
#define EDID_BLOCK_SIZE           128

static const uint8_t edid_base[128] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, // fixed header (0-7)
    0x6a, 0x12, // manufacturer ID (8-9)
    0x01, 0x00, // product code (10-11)
    0x42, 0x4b, 0x1d, 0x00, // serial number (12-15)
    0x01, // week of manufacture (16)
    0x20, // year of manufacture (17)
    0x01, // EDID version (18)
    0x03, // EDID revision (19)
    EDID_VID_IN_PARAM, // video input parameter (20)
    0x00, // horizontal screen size in cm (21)
    0x00, // vertical screen size in cm (22)
    0x78, // display gamma (23)
    0x0a, // supported feature (24)
    0xee, 0x95, 0xa3, 0x54, 0x4c, 0x99, 0x26, 0x0f, 0x50, 0x54, // chromatic (25-34)
    0x00, 0x00, 0x00, // established timing (35-37)
    0x01, 0x01, // unused standard timing (38-39)
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, // unused standard timing
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, // unused standard timing continued
    // descriptor 1 (54-71)
    0x00, 0x00, // pixel clock in 10kHz
    0x00, // HACT LSB
    0x00, // VBLK LSB
    0x00, // HACT MSB | HBLK MSB
    0x00, // VACT LSB
    0x00, // VBLK LSB
    0x00, // VACT MSB | VBLK MSB
    0x00, // HFP LSB
    0x00, // HSYNC LSB
    0x00, // VFP LSB | VSYNC LSB
    0x00, // HFP MSB | HSYNC MSB | VFP MSB | VSYNC MSB
    0x00, // Horizontal size in mm LSB
    0x00, // Vertical size in mm LSB
    0x00, // HSIZE MSB | VSIZE LSB
    0x00, // Horizontal border pixels
    0x00, // Vertical border lines
    0x1e, // Features bitmap
    // descriptor 2 (72-89) display name
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x50, 0x61, 0x70, 0x65, 0x72, 0x20,
    0x4d, 0x6f, 0x6e, 0x69, 0x74, 0x6f, 0x72,
    // descriptor 3 (90-107) display range limits
    0x00, 0x00, 0x00, 0xfd, 0x00, 0x30, 0x55, 0x1e, 0x5a, 0x11, 0x00,
    0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    // descriptor 4 (108-125) dummy
    0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, // number of extensions (126)
    0x00 // checksum (127)
};

static uint8_t edid[EDID_BLOCK_SIZE];
static uint8_t hdmi_edid[EDID_BLOCK_SIZE];

static uint8_t edid_size_cm(uint16_t size_mm) {
    uint16_t size_cm = size_mm / 10;

    if ((size_mm != 0) && (size_cm < 10)) {
        size_cm = 10;
    }
    if (size_cm > 255) {
        size_cm = 255;
    }

    return size_cm;
}

static void edid_copy_base(uint8_t *dst) {
    for (int i = 0; i < EDID_BLOCK_SIZE; i++) {
        dst[i] = edid_base[i];
    }
}

static void edid_write_dtd(uint8_t *dst, int offset, const config_t *cfg) {
    uint32_t pclk_10khz_value = cfg->pclk_hz / 10000u;
    if (pclk_10khz_value > 0xffffu)
        pclk_10khz_value = 0xffffu;
    uint16_t pclk_10khz = pclk_10khz_value;

    dst[offset + 0] = pclk_10khz & 0xff;
    dst[offset + 1] = (pclk_10khz >> 8) & 0xff;
    dst[offset + 2] = cfg->hact & 0xff;
    dst[offset + 3] = cfg->hblk & 0xff;
    dst[offset + 4] = ((cfg->hact >> 8) << 4) | (cfg->hblk >> 8);
    dst[offset + 5] = cfg->vact & 0xff;
    dst[offset + 6] = cfg->vblk & 0xff;
    dst[offset + 7] = ((cfg->vact >> 8) << 4) | (cfg->vblk >> 8);
    dst[offset + 8] = cfg->hfp & 0xff;
    dst[offset + 9] = cfg->hsync & 0xff;
    dst[offset + 10] = ((cfg->vfp & 0xf) << 4) | (cfg->vsync & 0xf);
    dst[offset + 11] = ((cfg->hfp >> 8) << 6) | ((cfg->hsync >> 8) << 4) |
            ((cfg->vfp >> 4) << 2) | (cfg->vsync >> 4);
    dst[offset + 12] = cfg->size_x_mm & 0xff;
    dst[offset + 13] = cfg->size_y_mm & 0xff;
    dst[offset + 14] = ((cfg->size_x_mm >> 8) << 4) |
            ((cfg->size_y_mm >> 8));
    dst[offset + 15] = 0x00;
    dst[offset + 16] = 0x00;
    dst[offset + 17] = 0x1e;
}

static void edid_fix_checksum(uint8_t *dst) {
    uint8_t checksum = 0;

    dst[EDID_BLOCK_SIZE - 1] = 0;
    for (int i = 0; i < EDID_BLOCK_SIZE - 1; i++) {
        checksum += dst[i];
    }
    checksum = ~checksum + 1;
    dst[EDID_BLOCK_SIZE - 1] = checksum;
}

static void edid_fill(uint8_t *dst, const config_t *cfg) {
    edid_copy_base(dst);

    // Fill in runtime info
    dst[16] = cfg->mfg_week;
    dst[17] = cfg->mfg_year;
    dst[21] = edid_size_cm(cfg->size_x_mm);
    dst[22] = edid_size_cm(cfg->size_y_mm);
    edid_write_dtd(dst, EDID_DTD_PREFERRED_OFFSET, cfg);

    uint32_t devid = board_get_uid();
    // Populate serial number with this ID
    dst[12] = (devid >> 24) & 0xff;
    dst[13] = (devid >> 16) & 0xff;
    dst[14] = (devid >> 8) & 0xff;
    dst[15] = (devid) & 0xff;

    edid_fix_checksum(dst);
}

void edid_init(void) {
    edid_fill(edid, &config);
    edid_fill(hdmi_edid, &config);
}

uint8_t *edid_get_raw(void) {
    return edid;
}

uint8_t *edid_get_raw_hdmi(void) {
    return hdmi_edid;
}

uint16_t edid_get_raw_hdmi_size(void) {
    return EDID_BLOCK_SIZE;
}
