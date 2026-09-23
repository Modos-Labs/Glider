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
#ifdef GLIDER_HOST_TEST
#include <string.h>
#include "config.h"
#else
#include "platform.h"
#include "board.h"
#include "app.h"
#endif

config_t config;

static int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}

static bool is_valid_update_mode(int mode) {
    // Everything the USB protocol can legally set and the firmware can render
    // without extra state. UM_MANUAL_LUT_* (0/1) are excluded: they need a
    // host-uploaded LUT, and persisting them would boot the panel into a
    // mode the firmware cannot reproduce. Modes 2 and 7 were previously
    // rejected here, silently resetting a saved host-set mode at boot.
    return (mode == UM_FAST_MONO_NO_DITHER) ||
           (mode == UM_FAST_MONO_BAYER) ||
           (mode == UM_FAST_MONO_BLUE_NOISE) ||
           (mode == UM_FAST_GREY) ||
           (mode == UM_AUTO_LUT_NO_DITHER) ||
           (mode == UM_AUTO_LUT_ERROR_DIFFUSION);
}

static int default_osd_scale_2x_from_ppi(void) {
    if ((config.size_x_mm == 0) || (config.size_y_mm == 0))
        return 0;

    uint32_t ppi_x10 = ((uint32_t)config.hact * 254u) / config.size_x_mm;
    uint32_t ppi_y10 = ((uint32_t)config.vact * 254u) / config.size_y_mm;
    return (((ppi_x10 + ppi_y10) / 2u) > 2000u) ? 1 : 0;
}

void config_reset_button_actions(void) {
    config.button_actions[0] = ACT_NEXT_MODE;
    config.button_actions[1] = ACT_PREV_MODE;
    config.button_actions[2] = ACT_OPEN_SETTINGS;
    config.button_actions[3] = ACT_POFF;
    config.button_actions[4] = ACT_CLEAR;
    config.button_actions[5] = ACT_TOGGLE_AC;
}

static void config_init_settings(void) {
    config.schema_version = CONFIG_SCHEMA_VERSION;
    config_reset_button_actions();
    config.update_mode = UM_FAST_MONO_BAYER;
    config.lightness = 0;
    config.contrast = 0;
    config.reserved_tone = 0;
    config.autoclear_mode = AC_OFF;
    config.autoclear_interval = AC_5MIN;
    config.autoclear_threshold = AC_THRES_MED;
    config.osd_scale_2x = 0;
    for (int i = 0; i < TONE_MODE_COUNT; i++) {
        config.tone_lightness[i] = TONE_UNSET;
        config.tone_contrast[i] = TONE_UNSET;
    }
}

void config_init(void) {
    // Set default values
    config.size_x_mm = 270;
    config.size_y_mm = 203;
    config.mfg_week = 1;
    config.mfg_year = 0x20;

    // 1600x1200 @ 60
//    config.pclk_hz = 162000000;
//    config.hact = 1600;
//    config.vact = 1200;
//    config.hblk = 560;
//    config.hfp = 64;
//    config.hsync = 192;
//    config.vblk = 50;
//    config.vfp = 1;
//    config.vsync = 3;
//    config.tcon_vfp = 45;
//    config.tcon_vsync = 1;
//    config.tcon_vbp = 2;
//    config.tcon_vact = 1200;
//    config.tcon_hfp = 120;
//    config.tcon_hsync = 10;
//    config.tcon_hbp = 10;
//    config.tcon_hact = 400;

    // 1600x1200 @ 75
//    config.pclk_hz = 156618000;
//    config.hact = 1600;
//    config.vact = 1200;
//    config.hblk = 80;
//    config.hfp = 8;
//    config.hsync = 32;
//    config.vblk = 43;
//    config.vfp = 29;
//    config.vsync = 8;
//
//    config.tcon_vfp = 11;
//    config.tcon_vsync = 1;
//    config.tcon_vbp = 2;
//    config.tcon_vact = 1200;
//    // HFP + HSYNC + HBP = Incoming HBLK / 4
//    config.tcon_hfp = 16;
//    config.tcon_hsync = 2;
//    config.tcon_hbp = 2;
//    config.tcon_hact = 400;


    // 1448x1072 @ 75
    // config.pclk_hz = 127320000;
    // config.hact = 1448;
    // config.vact = 1072;
    // config.hblk = 80;
    // config.hfp = 8;
    // config.hsync = 32;
    // config.vblk = 39;
    // config.vfp = 25;
    // config.vsync = 8;

    // config.tcon_vfp = 11;
    // config.tcon_vsync = 1;
    // config.tcon_vbp = 2;
    // config.tcon_vact = 1072;
    // // HFP + HSYNC + HBP = Incoming HBLK / 4
    // config.tcon_hfp = 17;
    // config.tcon_hsync = 2;
    // config.tcon_hbp = 1;
    // config.tcon_hact = 362;

//    config.pclk_hz = 72509000;
//    config.hact = 1040;
//    config.vact = 1040;
//    config.hblk = 80;
//    config.hfp = 8;
//    config.hsync = 32;
//    config.vblk = 39;
//    config.vfp = 25;
//    config.vsync = 8;
//
//    config.tcon_vfp = 11;
//    config.tcon_vsync = 1;
//    config.tcon_vbp = 2;
//    config.tcon_vact = 1040;
//    // HFP + HSYNC + HBP = Incoming HBLK / 4
//    config.tcon_hfp = 17;
//    config.tcon_hsync = 2;
//    config.tcon_hbp = 1;
//    config.tcon_hact = 260;

//    config.pclk_hz = 162000000;
//    config.hact = 1872;
//    config.vact = 1404;
//    config.hblk = 40;
//    config.hfp = 8;
//    config.hsync = 16;
//    config.vblk = 6;
//    config.vfp = 2;
//    config.vsync = 2;
//
//    config.tcon_vfp = 1;
//    config.tcon_vsync = 1;
//    config.tcon_vbp = 2;
//    config.tcon_vact = 1404;
//    // HFP + HSYNC + HBP = Incoming HBLK / 4
//    config.tcon_hfp = 7;
//    config.tcon_hsync = 2;
//    config.tcon_hbp = 1;
//    config.tcon_hact = 468;
//
//    config.vcom = -2.17f;
//    config.vgh = 22.0f;
//
//    config.mirror = 0;

    // 2232x1680 @ 40
//    config.pclk_hz = 158873000;
//    config.hact = 2240;
//    config.vact = 1680;
//    config.hblk = 80;
//    config.hfp = 8;
//    config.hsync = 32;
//    config.vblk = 32;
//    config.vfp = 18;
//    config.vsync = 8;
//
//    config.tcon_vfp = 12;
//    config.tcon_vsync = 1;
//    config.tcon_vbp = 1;
//    config.tcon_vact = 1680;
//    // HFP + HSYNC + HBP = Incoming HBLK / 4
//    config.tcon_hfp = 16;
//    config.tcon_hsync = 2;
//    config.tcon_hbp = 2;
//    config.tcon_hact = 560;
//
//    config.vcom = -0.8f;
//    config.vgh = 22.0f;

    // 2200x1648 @ 42
//    config.pclk_hz = 160972000;
//    config.hact = 2200;
//    config.vact = 1648;
//    config.hblk = 80;
//    config.hfp = 8;
//    config.hsync = 32;
//    config.vblk = 33;
//    config.vfp = 19;
//    config.vsync = 8;
//
//    config.tcon_vfp = 11;
//    config.tcon_vsync = 1;
//    config.tcon_vbp = 2;
//    config.tcon_vact = 1648;
//    config.tcon_hfp = 16;
//    config.tcon_hsync = 2;
//    config.tcon_hbp = 2;
//    config.tcon_hact = 550;

//    config.pclk_hz = 51500000;
//    config.hact = 800;
//    config.vact = 480;
//    config.hblk = 80;
//    config.hfp = 8;
//    config.hsync = 32;
//    config.vblk = 8;
//    config.vfp = 2;
//    config.vsync = 2;
//
//    config.tcon_vfp = 4;
//    config.tcon_vsync = 1;
//    config.tcon_vbp = 2;
//    config.tcon_vact = 480;
//    config.tcon_hfp = 4;
//    config.tcon_hsync = 4;
//    config.tcon_hbp = 12;
//    config.tcon_hact = 200;

    config.vcom = -2.30f;
    config.vgh = 25.0f;

    //config.mirror = 1;

    config.input_sel = INPUT_SEL_AUTO;
    strncpy(config.bitstream, "fpga.bit", BITSTREAM_NAME_MAX);
    config_init_settings();
}

void config_validate_loaded(size_t loaded_size) {
    if (loaded_size <= offsetof(config_t, schema_version)) {
        config_init_settings();
        config.osd_scale_2x = default_osd_scale_2x_from_ppi();
        return;
    }

    config.schema_version = CONFIG_SCHEMA_VERSION;

    const int default_actions[CONFIG_BUTTON_BINDING_COUNT] = {
        ACT_NEXT_MODE,
        ACT_PREV_MODE,
        ACT_OPEN_SETTINGS,
        ACT_POFF,
        ACT_CLEAR,
        ACT_TOGGLE_AC,
    };
    for (int i = 0; i < CONFIG_BUTTON_BINDING_COUNT; i++) {
        if ((config.button_actions[i] < 0) || (config.button_actions[i] >= ACT_COUNT))
            config.button_actions[i] = default_actions[i];
    }

    if (!is_valid_update_mode(config.update_mode))
        config.update_mode = UM_FAST_MONO_BAYER;
    if (config.input_sel > INPUT_SEL_DP)
        config.input_sel = INPUT_SEL_AUTO;

    config.lightness = clamp_int(config.lightness, -3, 3);
    config.contrast = clamp_int(config.contrast, -1, 6);
    config.reserved_tone = 0;

    if ((config.autoclear_mode < 0) || (config.autoclear_mode >= AC_MODE_COUNT))
        config.autoclear_mode = AC_OFF;
    if ((config.autoclear_interval < 0) || (config.autoclear_interval >= AC_INTERVAL_COUNT))
        config.autoclear_interval = AC_5MIN;
    if ((config.autoclear_threshold < 0) || (config.autoclear_threshold >= AC_THRES_COUNT))
        config.autoclear_threshold = AC_THRES_MED;

    if (loaded_size <= offsetof(config_t, osd_scale_2x))
        config.osd_scale_2x = default_osd_scale_2x_from_ppi();
    else
        config.osd_scale_2x = clamp_int(config.osd_scale_2x, 0, 1);

    // Tone arrays are appended to config_t: legacy saves are shorter and
    // leave the init values (TONE_UNSET) in place, which is exactly the
    // "no tone stored for this mode" state.
    for (int i = 0; i < TONE_MODE_COUNT; i++) {
        if ((config.tone_lightness[i] == TONE_UNSET) ||
                (config.tone_contrast[i] == TONE_UNSET)) {
            config.tone_lightness[i] = TONE_UNSET;
            config.tone_contrast[i] = TONE_UNSET;
        }
        else {
            config.tone_lightness[i] = clamp_int(config.tone_lightness[i], -3, 3);
            config.tone_contrast[i] = clamp_int(config.tone_contrast[i], -1, 6);
        }
    }
}

#ifndef GLIDER_HOST_TEST
void config_load(void) {
    config_init();

    SPIFFS_clearerr(&spiffs_fs);
    spiffs_file f = SPIFFS_open(&spiffs_fs, "config.bin", SPIFFS_O_RDONLY, 0);
    if (SPIFFS_errno(&spiffs_fs) != 0)
        return;
    
    spiffs_stat s;
    SPIFFS_fstat(&spiffs_fs, f, &s);
    uint32_t size = s.size;
    if (size > sizeof(config)) {
        SPIFFS_close(&spiffs_fs, f);
        return;
    }

    SPIFFS_read(&spiffs_fs, f, &config, size);
    SPIFFS_close(&spiffs_fs, f);
    config_validate_loaded(size);
}

void config_save(void) {
    SPIFFS_clearerr(&spiffs_fs);
    spiffs_file f = SPIFFS_open(&spiffs_fs, "config.bin", SPIFFS_O_CREAT | SPIFFS_O_TRUNC | SPIFFS_O_WRONLY, 0);
    if (SPIFFS_errno(&spiffs_fs) != 0) {
        syslog_printf("config_save: open failed: %d\n", SPIFFS_errno(&spiffs_fs));
        return;
    }

    int32_t res = SPIFFS_write(&spiffs_fs, f, &config, sizeof(config));
    if (res != sizeof(config)) {
        syslog_printf("config_save: write failed: %d (res %d)\n",
                SPIFFS_errno(&spiffs_fs), (int)res);
    }
    // Close must not be skipped: SPIFFS flushes cached writes to flash only
    // on close, and an unclosed fd leaks one of the 32 fd slots per save.
    res = SPIFFS_close(&spiffs_fs, f);
    if (res != SPIFFS_OK) {
        syslog_printf("config_save: close failed: %d\n", SPIFFS_errno(&spiffs_fs));
    }
}

// Set by tasks that want a config save without doing flash work in their
// own context (e.g. the USB handler); consumed by the UI task, which
// coalesces bursts into a single write.
volatile bool config_save_pending = false;

void config_request_save(void) {
    config_save_pending = true;
}
#else
void config_load(void) {
    config_init();
}

void config_save(void) {
}

void config_request_save(void) {
}
#endif

// Legacy configs (shorter than the tone arrays) leave the arrays at their
// init values, so no migration is needed: TONE_UNSET means "nothing stored
// for this mode" and such modes keep whatever tone is currently active.
void config_note_tone_for_mode(update_mode_t mode) {
    if ((unsigned)mode >= TONE_MODE_COUNT)
        return;
    config.tone_lightness[mode] = config.lightness;
    config.tone_contrast[mode] = config.contrast;
}

bool config_recall_tone_for_mode(update_mode_t mode) {
    if ((unsigned)mode >= TONE_MODE_COUNT)
        return false;
    if (config.tone_lightness[mode] == TONE_UNSET)
        return false;
    if ((config.lightness == config.tone_lightness[mode]) &&
            (config.contrast == config.tone_contrast[mode]))
        return false;
    config.lightness = config.tone_lightness[mode];
    config.contrast = config.tone_contrast[mode];
    return true;
}

