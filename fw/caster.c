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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "config.h"
#include "caster.h"
#include "fpga.h"

static size_t last_update;
static size_t last_update_duration;
static uint8_t waveform_frames;

static uint8_t get_update_frames(void) {
    // Should be worst case time to clear/ update a frame
    //uint8_t min_time = 10; // Minimum time for non-LUT modes
    // actually, just always return 1s
    return 60;
}

static void wait(void) {
    // Reading is not implemented in the simulator
}

void caster_init(void) {
    waveform_frames = 38; // Need to sync with the RTL code
    // fpga_write_reg8(CSR_CFG_V_FP, TCON_VFP);
    // fpga_write_reg8(CSR_CFG_V_SYNC, TCON_VSYNC);
    // fpga_write_reg8(CSR_CFG_V_BP, TCON_VBP);
    // fpga_write_reg16(CSR_CFG_V_ACT, TCON_VACT);
    // fpga_write_reg8(CSR_CFG_H_FP, TCON_HFP);
    // fpga_write_reg8(CSR_CFG_H_SYNC, TCON_HSYNC);
    // fpga_write_reg8(CSR_CFG_H_BP, TCON_HBP);
    // fpga_write_reg16(CSR_CFG_H_ACT, TCON_HACT);
    // fpga_write_reg8(CSR_CONTROL, 1); // Enable refresh
    //fpga_write_reg8(CSR_CFG_MINDRV, 0);
    fpga_write_reg8(CSR_LUT_FRAME, 38);
}

void caster_load_waveform(uint8_t *waveform, uint8_t frames) {
    wait();
    fpga_write_reg8(CSR_LUT_FRAME, 0); // Reset value before loading
    fpga_write_reg16(CSR_LUT_ADDR, 0);
    fpga_write_bulk(CSR_LUT_WR, waveform, WAVEFORM_SIZE);
    waveform_frames = frames;
}

void caster_redraw(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    wait();
    fpga_write_reg16(CSR_OP_LEFT, x0);
    fpga_write_reg16(CSR_OP_TOP, y0);
    fpga_write_reg16(CSR_OP_RIGHT, x1);
    fpga_write_reg16(CSR_OP_BOTTOM, y1);
    fpga_write_reg8(CSR_OP_LENGTH, get_update_frames());
    fpga_write_reg8(CSR_OP_CMD, OP_EXT_REDRAW);
}

void caster_setmode(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        UPDATE_MODE mode) {
    wait();
    fpga_write_reg16(CSR_OP_LEFT, x0);
    fpga_write_reg16(CSR_OP_TOP, y0);
    fpga_write_reg16(CSR_OP_RIGHT, x1);
    fpga_write_reg16(CSR_OP_BOTTOM, y1);
    fpga_write_reg8(CSR_OP_LENGTH, get_update_frames());
    fpga_write_reg8(CSR_OP_PARAM, (uint8_t)mode);
    fpga_write_reg8(CSR_OP_CMD, OP_EXT_SETMODE);
}