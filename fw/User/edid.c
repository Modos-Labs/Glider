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
#include "platform.h"
#include "board.h"
#include "app.h"

// 0x81 for DVI, 0x85 for DP
// Always use 0x85 for now, doesn't really matter
#define EDID_VID_IN_PARAM   (0x85)

static uint8_t edid[128] = {
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
    0x06, // supported feature (24)
    0xee, 0x95, 0xa3, 0x54, 0x4c, 0x99, 0x26, 0x0f, 0x50, 0x54, // chromatic (25-34)
    0x00, 0x00, 0x00, // established timing (35-37)
    0x01, // X resolution (38)
    0x00, // aspect ratio and vertical frequency (39)
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, // standard timing
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, // standard timing continued
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
    // descriptor 3 (90-107) display name
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    // descriptor 4 (108-125) dummy
    0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, // number of extensions (126)
    0x00 // checksum (127)
};

void edid_init(void) {
    // Fill in runtime info
    edid[16] = config.mfg_week;
    edid[17] = config.mfg_year;
    edid[21] = config.size_x_mm / 10;
    edid[22] = config.size_y_mm / 10;
    edid[54] = (config.pclk_hz / 10000) & 0xff;
    edid[55] = ((config.pclk_hz / 10000) >> 8) & 0xff;
    edid[56] = config.hact & 0xff;
    edid[57] = config.hblk & 0xff;
    edid[58] = ((config.hact >> 8) << 4) | (config.hblk >> 8);
    edid[59] = config.vact & 0xff;
    edid[60] = config.vblk & 0xff;
    edid[61] = ((config.vact >> 8) << 4) | (config.vblk >> 8);
    edid[62] = config.hfp & 0xff;
    edid[63] = config.hsync & 0xff;
    edid[64] = ((config.vfp & 0xf) << 4) | (config.vsync & 0xf);
    edid[65] = ((config.hfp >> 8) << 6) | ((config.hsync >> 8) << 4) |
            ((config.vfp >> 4) << 2) | (config.vsync >> 4);
    edid[66] = config.size_x_mm & 0xff;
    edid[67] = config.size_y_mm & 0xff;
    edid[68] = ((config.size_x_mm >> 8) << 4) | ((config.size_y_mm >> 8));

    uint32_t devid = board_get_uid();
    // Populate serial number with this ID
    edid[12] = (devid >> 24) & 0xff;
    edid[13] = (devid >> 16) & 0xff;
    edid[14] = (devid >> 8) & 0xff;
    edid[15] = (devid) & 0xff;

    // Fix checksum in EDID
    uint8_t checksum = 0;
    for (int i = 0; i < 127; i++) {
        checksum += edid[i];
    }
    checksum = ~checksum + 1;
    edid[127] = checksum;
}

uint8_t *edid_get_raw() {
    return edid;
}
