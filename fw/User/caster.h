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
#define CSR_ENABLE          15
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
#define CSR_CFG_MINDRV      30
#define CSR_OSD_EN          31
#define CSR_OSD_LEFT_HI     32
#define CSR_OSD_LEFT_LO     33
#define CSR_OSD_RIGHT_HI    34
#define CSR_OSD_RIGHT_LO    35
#define CSR_OSD_TOP_HI      36
#define CSR_OSD_TOP_LO      37
#define CSR_OSD_BOTTOM_HI   38
#define CSR_OSD_BOTTOM_LO   39
#define CSR_OSD_ADDR_HI     40
#define CSR_OSD_ADDR_LO     41
#define CSR_OSD_WR          42
#define CSR_CFG_MIRROR      43
#define CSR_CFG_B2WFRAME    44
#define CSR_CFG_W2BFRAME    45
#define CSR_CFG_FASTG_B2GFRAME 46
#define CSR_CFG_FASTG_W2GFRAME 47
#define CSR_OSD_SCALE       48
#define CSR_TONE_ADDR       49
#define CSR_TONE_WR         50
#define CSR_INPUT_CTRL      51
#define CSR_STATUS          128
#define CSR_ID0             129
#define CSR_DAMAGE_COUNT_HI 130
#define CSR_DAMAGE_COUNT_MID 131
#define CSR_DAMAGE_COUNT_LO 132
#define CSR_INPUT_STATUS        133
#define CSR_INPUT_MEAS_HACT_HI  134
#define CSR_INPUT_MEAS_HACT_LO  135
#define CSR_INPUT_MEAS_VACT_HI  136
#define CSR_INPUT_MEAS_VACT_LO  137
#define CSR_INPUT_MEAS_HTOT_HI  138
#define CSR_INPUT_MEAS_HTOT_LO  139
#define CSR_INPUT_MEAS_VTOT_HI  140
#define CSR_INPUT_MEAS_VTOT_LO  141
#define CSR_DEBUG_MEMIF_STATE   142
#define CSR_DEBUG_FIFO_STATE    143
#define CSR_INPUT_DEBUG         144
// Alias for 16bit registers
#define CSR_LUT_ADDR        CSR_LUT_ADDR_HI
#define CSR_OP_LEFT         CSR_OP_LEFT_HI
#define CSR_OP_RIGHT        CSR_OP_RIGHT_HI
#define CSR_OP_TOP          CSR_OP_TOP_HI
#define CSR_OP_BOTTOM       CSR_OP_BOTTOM_HI
#define CSR_CFG_V_ACT       CSR_CFG_V_ACT_HI
#define CSR_CFG_H_ACT       CSR_CFG_H_ACT_HI
#define CSR_OSD_LEFT        CSR_OSD_LEFT_HI
#define CSR_OSD_RIGHT       CSR_OSD_RIGHT_HI
#define CSR_OSD_TOP         CSR_OSD_TOP_HI
#define CSR_OSD_BOTTOM      CSR_OSD_BOTTOM_HI
#define CSR_OSD_ADDR        CSR_OSD_ADDR_HI

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

#define INPUT_CTRL_INTERNAL     (1u << 0)
#define INPUT_CTRL_TMDS         (1u << 1)
#define INPUT_CTRL_DP           (1u << 2)
#define INPUT_CTRL_AUTO         (1u << 3)

#define INPUT_STATUS_INTERNAL   (1u << 0)
#define INPUT_STATUS_TMDS       (1u << 1)
#define INPUT_STATUS_DP         (1u << 2)
#define INPUT_STATUS_STABLE     (1u << 3)
#define INPUT_STATUS_SUPPORTED  (1u << 4)
#define INPUT_STATUS_LIVE       (1u << 5)
#define INPUT_STATUS_RESET      (1u << 6)
#define INPUT_STATUS_LOST       (1u << 7)

#define WAVEFORM_SIZE       (4*1024)

#define FRAME_RATE_HZ       (60)

#define CASTER_OSD_WIDTH    320
#define CASTER_OSD_HEIGHT   200
#define CASTER_OSD_STRIDE   (CASTER_OSD_WIDTH / 8)
#define CASTER_OSD_BUF_SIZE (CASTER_OSD_STRIDE * CASTER_OSD_HEIGHT)

typedef enum {
    UM_MANUAL_LUT_NO_DITHER = 0,
    UM_MANUAL_LUT_ERROR_DIFFUSION = 1,
    UM_FAST_MONO_NO_DITHER = 2,
    UM_FAST_MONO_BAYER = 3,
    UM_FAST_MONO_BLUE_NOISE = 4,
    UM_FAST_GREY = 5,
    UM_AUTO_LUT_NO_DITHER = 6,
    UM_AUTO_LUT_ERROR_DIFFUSION = 7
} update_mode_t;

void caster_init(void);
uint8_t caster_load_waveform(uint8_t *waveform, uint8_t frames);
uint8_t caster_redraw(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
uint8_t caster_setmode(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
    update_mode_t mode);
uint32_t caster_get_damage_counter(void);
uint8_t caster_is_busy(void);
void caster_redraw_blank(void);
uint8_t caster_osd_send_buf(uint8_t *buf);
uint8_t caster_osd_set_window(uint16_t left, uint16_t top,
    uint16_t logical_width, uint16_t logical_height);
uint8_t caster_osd_set_enable(bool en);
void caster_set_tone(int lightness, int contrast);
uint8_t caster_input_status(void);
uint8_t caster_input_debug(void);
void caster_input_force_internal(void);
void caster_input_request_tmds(void);
void caster_input_request_dp(void);
void caster_input_request_auto(void);
void caster_input_get_measured(uint16_t *hact, uint16_t *vact,
        uint16_t *htotal, uint16_t *vtotal);
