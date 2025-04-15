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

typedef struct {
    uint32_t pclk_hz; // pixel clock
    uint8_t hfp;
    uint8_t vfp;
    uint8_t hsync;
    uint8_t vsync;
    uint16_t hact;
    uint16_t hblk;
    uint16_t vact;
    uint16_t vblk;
    uint16_t size_x_mm; // horizontal screen size
    uint16_t size_y_mm; // vertical screen size
    uint8_t mfg_week;
    uint8_t mfg_year;
    float vcom;
    float vgh;
    // TCON configurations
    uint8_t tcon_vfp;
    uint8_t tcon_vsync;
    uint8_t tcon_vbp;
    uint16_t tcon_vact;
    uint8_t tcon_hfp;
    uint8_t tcon_hsync;
    uint8_t tcon_hbp;
    uint16_t tcon_hact;
} config_t;

extern config_t config;

void config_init(void);
void config_load(void);
void config_save(void);
