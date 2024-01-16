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
#include <stdint.h>
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <pico/unique_id.h>
#include <pico/i2c_slave.h>
#include "edid.h"
#include "config.h"

#ifdef INPUT_DVI
#define EDID_I2C_ADDRESS    (0x50)
#define EDID_I2C            (i2c0)
#define EDID_I2C_SDA        (0)
#define EDID_I2C_SCL        (1)
#endif

static char edid[129] = {
    0x00, // register number, not part of EDID
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, // fixed header (0-7)
    0x6a, 0x12, // manufacturer ID (8-9)
    0x01, 0x00, // product code (10-11)
    0x42, 0x4b, 0x1d, 0x00, // serial number (12-15)
    0x01, // week of manufacture (16)
    0x20, // year of manufacture (17)
    0x01, // EDID version (18)
    0x03, // EDID revision (19)
    0x85, // display parameter (20)
    (SCREEN_SIZE_X / 10), // horizontal screen size in cm (21)
    (SCREEN_SIZE_Y / 10), // vertical screen size in cm (22)
    0x78, // display gamma (23)
    0x06, // supported feature (24)
    0xee, 0x95, 0xa3, 0x54, 0x4c, 0x99, 0x26, 0x0f, 0x50, 0x54, // chromatic (25-34)
    0x00, 0x00, 0x00, // established timing (35-37)
    (SCREEN_HACT / 8 - 31), // X resolution (38)
    SCREEN_ASPECT, // aspect ratio and vertical frequency (39)
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, // standard timing
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, // standard timing continued
    // descriptor 1 (54-71)
    ((SCREEN_CLK / 10) & 0xff), ((SCREEN_CLK / 10) >> 8), // pixel clock in 10kHz
    (SCREEN_HACT & 0xff), // HACT LSB
    (SCREEN_HBLK & 0xff), // VBLK LSB
    (((SCREEN_HACT >> 8) << 4) | (SCREEN_HBLK >> 8)), // HACT MSB | HBLK MSB
    (SCREEN_VACT & 0xff), // VACT LSB
    (SCREEN_VBLK & 0xff), // VBLK LSB
    (((SCREEN_VACT >> 8) << 4) | (SCREEN_VBLK >> 8)), // VACT MSB | VBLK MSB
    (SCREEN_HFP & 0xff), // HFP LSB
    (SCREEN_HSYNC & 0xff), // HSYNC LSB
    (((SCREEN_VFP & 0xf) << 4) | (SCREEN_VSYNC & 0xf)), // VFP LSB | VSYNC LSB
    (((SCREEN_HFP >> 8) << 6) | ((SCREEN_HSYNC >> 8) << 4) |
        ((SCREEN_VFP >> 4) << 2) | (SCREEN_VSYNC >> 4)), // HFP MSB | HSYNC MSB | VFP MSB | VSYNC MSB
    (SCREEN_SIZE_X & 0xff), // Horizontal size in mm LSB
    (SCREEN_SIZE_Y & 0xff), // Vertical size in mm LSB
    (((SCREEN_SIZE_X >> 8) << 4) | (SCREEN_SIZE_Y >> 8)), // HSIZE MSB | VSIZE LSB
    0x00, // Horizontal border pixels
    0x00, // Vertical border lines 0x1e,
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

#ifdef INPUT_DVI
static void edid_i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    static uint8_t addr = 0;

    switch (event) {
    case I2C_SLAVE_RECEIVE:
        // I2C master has written some data
        // EDID is read only, all write are treated as address
        addr = i2c_read_byte_raw(i2c);
        break;
    case I2C_SLAVE_REQUEST:
        // I2C master is requesting data
        i2c_write_byte_raw(i2c, edid[addr + 1]);
        addr++;
        if (addr == 128)
            addr = 0; // wrap around
        break;
    case I2C_SLAVE_FINISH:
        // I2C master sent stop
        // Reset transaction if needed
        break;
    }
}
#endif

void edid_init() {
    // Fill in runtime info
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    // Populate serial number with this ID (XORed down to 4 bytes)
    edid[13] = board_id.id[0] ^ board_id.id[4];
    edid[14] = board_id.id[1] ^ board_id.id[5];
    edid[15] = board_id.id[2] ^ board_id.id[6];
    edid[16] = board_id.id[3] ^ board_id.id[7];

    // Fix checksum in EDID
    uint8_t checksum = 0;
    for (int i = 1; i < 128; i++) {
        checksum += edid[i];
    }
    checksum = ~checksum + 1;
    edid[128] = checksum;

#ifdef INPUT_DVI
    // DVI models has DDC I2C connected directly to the RP2040
    // EDID ROM emulation is needed

    gpio_init(EDID_I2C_SDA);
    gpio_init(EDID_I2C_SCL);

    gpio_set_function(EDID_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(EDID_I2C_SCL, GPIO_FUNC_I2C);

    i2c_slave_init(EDID_I2C, EDID_I2C_ADDRESS, &edid_i2c_slave_handler);

    // Pull HPD high so host can read the EDID

#endif
}

uint8_t *edid_get_raw() {
    return edid;
}
