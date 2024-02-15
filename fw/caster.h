//
// Caster simulator
// Copyright 2023 Wenting Zhang
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
#pragma once

// Register map
#define CSR_LUT_FRAME       0
#define CSR_LUT_ADDR_HI     1
#define CSR_LUT_ADDR_LO     2
#define CSR_LUT_WR          3
#define CSR_OP_LEFT_HI      4
#define CSR_OP_LEFT_LO      5
#define CSR_OP_RIGHT_HI     6
#define CSR_OP_RIGHT_LO     7
#define CSR_OP_TOP_HI       8
#define CSR_OP_TOP_LO       9
#define CSR_OP_BOTTOM_HI    10
#define CSR_OP_BOTTOM_LO    11
#define CSR_OP_PARAM        12
#define CSR_OP_LENGTH       13
#define CSR_OP_CMD          14
#define CSR_CONTROL         15
#define CSR_CFG_V_FP        16
#define CSR_CFG_V_SYNC      17
#define CSR_CFG_V_BP        18
#define CSR_CFG_V_ACT_HI    19
#define CSR_CFG_V_ACT_LO    20
#define CSR_CFG_H_FP        21
#define CSR_CFG_H_SYNC      22
#define CSR_CFG_H_BP        23
#define CSR_CFG_H_ACT_HI    24
#define CSR_CFG_H_ACT_LO    25
#define CSR_CFG_FBYTES_B2   27
#define CSR_CFG_FBYTES_B1   28
#define CSR_CFG_FBYTES_B0   29
#define CSR_STATUS          32
// Alias for 16bit registers
#define CSR_LUT_ADDR        CSR_LUT_ADDR_HI
#define CSR_OP_LEFT         CSR_OP_LEFT_HI
#define CSR_OP_RIGHT        CSR_OP_RIGHT_HI
#define CSR_OP_TOP          CSR_OP_TOP_HI
#define CSR_OP_BOTTOM       CSR_OP_BOTTOM_HI
#define CSR_CFG_V_ACT       CSR_CFG_V_ACT_HI
#define CSR_CFG_H_ACT       CSR_CFG_H_ACT_HI

// Commands
#define OP_EXT_REDRAW       0
#define OP_EXT_SETMODE      1

// Status bits
#define STATUS_MIG_ERROR    7
#define STATUS_MIF_ERROR    6
#define STATUS_SYS_READY    5
#define STATUS_OP_BUSY      4
#define STATUS_OP_QUEUE     3
#define CTRL_ENABLE         0

#define WAVEFORM_SIZE       (4*1024)

#define FRAME_RATE_HZ       (60)

typedef enum {
    UM_MANUAL_LUT_NO_DITHER = 0,
    UM_MANUAL_LUT_ERROR_DIFFUSION = 1,
    UM_FAST_MONO_NO_DITHER = 2,
    UM_FAST_MONO_BAYER = 3,
    UM_FAST_MONO_BLUE_NOISE = 4,
    UM_FAST_GREY = 5,
    UM_AUTO_LUT_NO_DITHER = 6,
    UM_AUTO_LUT_ERROR_DIFFUSION = 7
} UPDATE_MODE;

void caster_init(void);
uint8_t caster_load_waveform(uint8_t *waveform, uint8_t frames);
uint8_t caster_redraw(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
uint8_t caster_setmode(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        UPDATE_MODE mode);
