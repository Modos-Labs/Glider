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
#include "tone_lut.h"

static size_t last_update;
static size_t last_update_duration;
static uint8_t waveform_frames;

enum {
    CASTER_MIN_DRV = 2,
    CASTER_FASTM_B2W_FRAMES = 9,
    CASTER_FASTM_W2B_FRAMES = 9,
    CASTER_FASTG_B2G_FRAMES = 2,
    CASTER_FASTG_W2G_FRAMES = 2,
};

static uint8_t get_update_frames(void) {
    // Should be worst case time to clear/ update a frame
    //uint8_t min_time = 10; // Minimum time for non-LUT modes
    // actually, just always return 0.5s
    return 16;
}

static uint16_t osd_x_pixel_to_group(uint16_t pixels) {
    return (uint16_t)((pixels + 3u) / 4u);
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
    fpga_write_reg8(CSR_CFG_MINDRV, CASTER_MIN_DRV);
    fpga_write_reg8(CSR_CFG_B2WFRAME, CASTER_FASTM_B2W_FRAMES);
    fpga_write_reg8(CSR_CFG_W2BFRAME, CASTER_FASTM_W2B_FRAMES);
    fpga_write_reg8(CSR_CFG_FASTG_B2GFRAME, CASTER_FASTG_B2G_FRAMES);
    fpga_write_reg8(CSR_CFG_FASTG_W2GFRAME, CASTER_FASTG_W2G_FRAMES);
    fpga_write_reg8(CSR_LUT_FRAME, 38);
    caster_osd_set_window(0, 0, CASTER_OSD_WIDTH, CASTER_OSD_HEIGHT);
    fpga_write_reg8(CSR_OSD_EN, 0);
    fpga_write_reg8(CSR_CFG_MIRROR, config.mirror);
    caster_set_tone(config.lightness, config.contrast);
    fpga_write_reg8(CSR_ENABLE, 1); // Enable refresh
}

static uint8_t is_busy() {
    uint8_t status = fpga_write_reg8(CSR_STATUS, 0x00);
    return !!(status & STATUS_OP_QUEUE);
}

uint8_t caster_is_busy(void) {
    return is_busy();
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

uint32_t caster_get_damage_counter(void) {
    uint32_t damage = 0;
    damage |= (uint32_t)(fpga_write_reg8(CSR_DAMAGE_COUNT_HI, 0x00) & 0x1f) << 16;
    damage |= (uint32_t)fpga_write_reg8(CSR_DAMAGE_COUNT_MID, 0x00) << 8;
    damage |= (uint32_t)fpga_write_reg8(CSR_DAMAGE_COUNT_LO, 0x00);
    return damage;
}

uint8_t caster_setinput(uint8_t input_src) {
//    if (is_busy()) return 1;
//    fpga_write_reg8(CSR_CFG_IN_SRC, input_src);
    return 0;
}

void caster_redraw_blank(void) {
    fpga_write_reg16(CSR_OP_LEFT, 0);
    fpga_write_reg16(CSR_OP_TOP, 0);
    fpga_write_reg16(CSR_OP_RIGHT, config.hact);
    fpga_write_reg16(CSR_OP_BOTTOM, config.vact);
    fpga_write_reg8(CSR_OP_LENGTH, get_update_frames());
    fpga_write_reg8(CSR_ENABLE, 0x3); // Enable blanking
    fpga_write_reg8(CSR_OP_CMD, OP_EXT_REDRAW);
}

uint8_t caster_osd_send_buf(uint8_t *buf) {
    fpga_write_reg16(CSR_OSD_ADDR, 0);
    fpga_write_bulk(CSR_OSD_WR, buf, CASTER_OSD_BUF_SIZE);
    return 0;
}

uint8_t caster_osd_set_window(uint16_t left, uint16_t top,
        uint16_t logical_width, uint16_t logical_height) {
    uint16_t scale = config.osd_scale_2x ? 2u : 1u;
    uint16_t right = left + logical_width * scale;
    uint16_t bottom = top + logical_height * scale;

    fpga_write_reg8(CSR_OSD_SCALE, config.osd_scale_2x ? 1 : 0);
    fpga_write_reg16(CSR_OSD_LEFT, osd_x_pixel_to_group(left));
    fpga_write_reg16(CSR_OSD_RIGHT, osd_x_pixel_to_group(right));
    fpga_write_reg16(CSR_OSD_TOP, top);
    fpga_write_reg16(CSR_OSD_BOTTOM, bottom);
    return 0;
}

uint8_t caster_osd_set_enable(bool en) {
    fpga_write_reg8(CSR_OSD_EN, en);
    return 0;
}

void caster_set_tone(int lightness, int contrast) {
    uint8_t lut[TONE_LUT_SIZE];

    tone_lut_build_y(lightness, contrast, lut);
    fpga_write_reg8(CSR_TONE_ADDR, 0);
    fpga_write_bulk(CSR_TONE_WR, lut, TONE_LUT_SIZE);
}

uint8_t caster_input_status(void) {
    return fpga_write_reg8(CSR_INPUT_STATUS, 0x00);
}

uint8_t caster_input_debug(void) {
    return fpga_write_reg8(CSR_INPUT_DEBUG, 0x00);
}

void caster_input_force_internal(void) {
    fpga_write_reg8(CSR_INPUT_CTRL, INPUT_CTRL_INTERNAL);
}

void caster_input_request_tmds(void) {
    fpga_write_reg8(CSR_INPUT_CTRL, INPUT_CTRL_TMDS);
}

void caster_input_request_dp(void) {
    fpga_write_reg8(CSR_INPUT_CTRL, INPUT_CTRL_DP);
}

void caster_input_request_auto(void) {
    fpga_write_reg8(CSR_INPUT_CTRL, INPUT_CTRL_AUTO);
}

void caster_input_get_measured(uint16_t *hact, uint16_t *vact,
        uint16_t *htotal, uint16_t *vtotal) {
    uint16_t hact_value = ((uint16_t)fpga_write_reg8(CSR_INPUT_MEAS_HACT_HI, 0x00) << 8) |
            fpga_write_reg8(CSR_INPUT_MEAS_HACT_LO, 0x00);
    uint16_t vact_value = ((uint16_t)fpga_write_reg8(CSR_INPUT_MEAS_VACT_HI, 0x00) << 8) |
            fpga_write_reg8(CSR_INPUT_MEAS_VACT_LO, 0x00);
    uint16_t htotal_value = ((uint16_t)fpga_write_reg8(CSR_INPUT_MEAS_HTOT_HI, 0x00) << 8) |
            fpga_write_reg8(CSR_INPUT_MEAS_HTOT_LO, 0x00);
    uint16_t vtotal_value = ((uint16_t)fpga_write_reg8(CSR_INPUT_MEAS_VTOT_HI, 0x00) << 8) |
            fpga_write_reg8(CSR_INPUT_MEAS_VTOT_LO, 0x00);

    if (hact != NULL)
        *hact = hact_value << 1;
    if (vact != NULL)
        *vact = vact_value;
    if (htotal != NULL)
        *htotal = htotal_value << 1;
    if (vtotal != NULL)
        *vtotal = vtotal_value;
}
