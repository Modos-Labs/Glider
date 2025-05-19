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

config_t config;

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
    config.pclk_hz = 156618000;
    config.hact = 1600;
    config.vact = 1200;
    config.hblk = 80;
    config.hfp = 8;
    config.hsync = 32;
    config.vblk = 43;
    config.vfp = 29;
    config.vsync = 8;

    config.tcon_vfp = 11;
    config.tcon_vsync = 1;
    config.tcon_vbp = 2;
    config.tcon_vact = 1200;
    // HFP + HSYNC + HBP = Incoming HBLK / 4
    config.tcon_hfp = 16;
    config.tcon_hsync = 2;
    config.tcon_hbp = 2;
    config.tcon_hact = 400;


    // 1448x1072 @ 75
//    config.pclk_hz = 127320000;
//    config.hact = 1448;
//    config.vact = 1072;
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
//    config.tcon_vact = 1072;
//    // HFP + HSYNC + HBP = Incoming HBLK / 4
//    config.tcon_hfp = 17;
//    config.tcon_hsync = 2;
//    config.tcon_hbp = 1;
//    config.tcon_hact = 362;

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
//
//    config.vcom = -2.45f;
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
//
//    config.mirror = 1;
}

void config_load(void) {
    SPIFFS_clearerr(&spiffs_fs);
    spiffs_file f = SPIFFS_open(&spiffs_fs, "config.bin", SPIFFS_O_RDONLY, 0);
    if (SPIFFS_errno(&spiffs_fs) != 0)
        return;
    
    spiffs_stat s;
    SPIFFS_fstat(&spiffs_fs, f, &s);
    uint32_t size = s.size;
    if (size != sizeof(config)) {
        SPIFFS_close(&spiffs_fs, f);
        return;
    }

    SPIFFS_read(&spiffs_fs, f, &config, sizeof(config));
    SPIFFS_close(&spiffs_fs, f);
}

void config_save(void) {
    SPIFFS_clearerr(&spiffs_fs);
    spiffs_file f = SPIFFS_open(&spiffs_fs, "config.bin", SPIFFS_O_CREAT | SPIFFS_O_TRUNC | SPIFFS_O_WRONLY, 0);
    if (SPIFFS_errno(&spiffs_fs) != 0)
        return;
    
    SPIFFS_write(&spiffs_fs, f, &config, sizeof(config));
}
