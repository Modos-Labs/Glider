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
    config.pclk_hz = 162000000;
    config.hact = 1600;
    config.vact = 1200;
    config.hblk = 560;
    config.hfp = 64;
    config.hsync = 192;
    config.vblk = 50;
    config.vfp = 1;
    config.vsync = 3;
    config.size_x_mm = 270;
    config.size_y_mm = 203;
    config.mfg_week = 1;
    config.mfg_year = 0x20;
    config.tcon_vfp = 45;
    config.tcon_vsync = 1;
    config.tcon_vbp = 2;
    config.tcon_vact = 1200;
    config.tcon_hfp = 120;
    config.tcon_hsync = 10;
    config.tcon_hbp = 10;
    config.tcon_hact = 400;
    config.vcom = -2.45f;
    config.vgh = 22.0f;
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