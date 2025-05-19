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

static size_t last_update;
static size_t last_update_duration;
static uint8_t waveform_frames;

static uint8_t get_update_frames(void) {
    // Should be worst case time to clear/ update a frame
    //uint8_t min_time = 10; // Minimum time for non-LUT modes
    // actually, just always return 0.5s
    return 16;
}

static void wait(void) {
    // Reading is not implemented in the simulator
}

void caster_init(void) {
    waveform_frames = 38; // Need to sync with the RTL code
    fpga_write_reg8(CSR_CFG_V_FP, config.tcon_vfp);
    fpga_write_reg8(CSR_CFG_V_SYNC, config.tcon_vsync);
    fpga_write_reg8(CSR_CFG_V_BP, config.tcon_vbp);
    fpga_write_reg16(CSR_CFG_V_ACT, config.tcon_vact);
    fpga_write_reg8(CSR_CFG_H_FP, config.tcon_hfp);
    fpga_write_reg8(CSR_CFG_H_SYNC, config.tcon_hsync);
    fpga_write_reg8(CSR_CFG_H_BP, config.tcon_hbp);
    fpga_write_reg16(CSR_CFG_H_ACT, config.tcon_hact);
    uint32_t frame_bytes = config.tcon_hact * 4 * config.tcon_vact * 2;
    fpga_write_reg8(CSR_CFG_FBYTES_B0, frame_bytes & 0xff);
    fpga_write_reg8(CSR_CFG_FBYTES_B1, (frame_bytes >> 8) & 0xff);
    fpga_write_reg8(CSR_CFG_FBYTES_B2, (frame_bytes >> 16) & 0xff);
    fpga_write_reg8(CSR_CFG_MINDRV, 2);
    fpga_write_reg8(CSR_LUT_FRAME, 38);
    fpga_write_reg16(CSR_OSD_LEFT, 0);
    fpga_write_reg16(CSR_OSD_RIGHT, 256/4);
    fpga_write_reg16(CSR_OSD_TOP, 0);
    fpga_write_reg16(CSR_OSD_BOTTOM, 128);
    fpga_write_reg8(CSR_OSD_EN, 0);
    fpga_write_reg8(CSR_CFG_MIRROR, config.mirror);
    fpga_write_reg8(CSR_ENABLE, 1); // Enable refresh
}

static uint8_t is_busy() {
    uint8_t status = fpga_write_reg8(CSR_STATUS, 0x00);
    return !!(status & STATUS_OP_QUEUE);
}

uint8_t caster_load_waveform(uint8_t *waveform, uint8_t frames) {
    fpga_write_reg8(CSR_LUT_FRAME, 0); // Reset value before loading
    fpga_write_reg16(CSR_LUT_ADDR, 0);
    fpga_write_bulk(CSR_LUT_WR, waveform, WAVEFORM_SIZE);
    waveform_frames = frames;
    return 0;
}

uint8_t caster_redraw(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    //if (is_busy()) return 1;
    fpga_write_reg16(CSR_OP_LEFT, x0);
    fpga_write_reg16(CSR_OP_TOP, y0);
    fpga_write_reg16(CSR_OP_RIGHT, x1);
    fpga_write_reg16(CSR_OP_BOTTOM, y1);
    fpga_write_reg8(CSR_OP_LENGTH, get_update_frames());
    fpga_write_reg8(CSR_OP_CMD, OP_EXT_REDRAW);
    return 0;
}

uint8_t caster_setmode(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        update_mode_t mode) {
    //if (is_busy()) return 1;
    fpga_write_reg16(CSR_OP_LEFT, x0);
    fpga_write_reg16(CSR_OP_TOP, y0);
    fpga_write_reg16(CSR_OP_RIGHT, x1);
    fpga_write_reg16(CSR_OP_BOTTOM, y1);
    fpga_write_reg8(CSR_OP_LENGTH, get_update_frames());
    fpga_write_reg8(CSR_OP_PARAM, (uint8_t)mode);
    fpga_write_reg8(CSR_OP_CMD, OP_EXT_SETMODE);
    return 0;
}

uint8_t caster_setinput(uint8_t input_src) {
//    if (is_busy()) return 1;
//    fpga_write_reg8(CSR_CFG_IN_SRC, input_src);
    return 0;
}

uint8_t caster_osd_send_buf(uint8_t *buf) {
    fpga_write_reg16(CSR_OSD_ADDR, 0);
    fpga_write_bulk(CSR_OSD_WR, buf, 4096);
    return 0;
}

uint8_t caster_osd_set_enable(bool en) {
    fpga_write_reg8(CSR_OSD_EN, en);
    return 0;
}
