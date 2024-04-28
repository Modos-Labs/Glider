//
// Copyright 2023 Wenting Zhang <zephray@outlook.com>
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

/* BOARD REVISION CONFIGURATION */
// Eariler revisions are not supported
#define BOARD_REV_R0P7

/* SCREEN CONFIGURATION */

// #define SCREEN_6_0_INCH
// #define SCREEN_7_8_INCH
// #define SCREEN_10_3_INCH
// #define SCREEN_10_8_INCH
#define SCREEN_13_3_INCH

// #define SCREEN_600_800
// #define SCREEN_800_600
// #define SCREEN_1024_758
// #define SCREEN_1448_1072
#define SCREEN_1600_1200
// #define SCREEN_1872_1404
// #define SCREEN_1920_1080
// #define SCREEN_2200_1650

#define SCREEN_MONO
// #define SCREEN_DES_COLOR


/* SET BASED ON PREVIOUS DEFINES, DO NOT MODIFY */
#if defined(BOARD_REV_R0P7)
#define POWER_GPIO
#define POWER_GPIO_VCOM_MEASURE
#define INPUT_ADV7611
#define INPUT_PTN3460
#define HAS_TYPEC
#else
#error "Unknown board revision"
#endif

// Used in EDID
#define SCREEN_ASPECT_16_10     0x00
#define SCREEN_ASPECT_4_3       0x40
#define SCREEN_ASPECT_5_4       0x80
#define SCREEN_ASPECT_16_9      0xC0

// Screen dimension in mm
#if defined(SCREEN_6_0_INCH)
#define SCREEN_SIZE_X   122
#define SCREEN_SIZE_Y   91
#define SCREEN_ASPECT   SCREEN_ASPECT_4_3
#elif defined(SCREEN_7_8_INCH)
#define SCREEN_SIZE_X   158
#define SCREEN_SIZE_Y   119
#define SCREEN_ASPECT   SCREEN_ASPECT_4_3
#elif defined(SCREEN_10_3_INCH)
#define SCREEN_SIZE_X   209
#define SCREEN_SIZE_Y   157
#define SCREEN_ASPECT   SCREEN_ASPECT_4_3
#elif defined(SCREEN_10_8_INCH)
#define SCREEN_SIZE_X   239
#define SCREEN_SIZE_Y   134
#define SCREEN_ASPECT   SCREEN_ASPECT_16_9
#elif defined(SCREEN_13_3_INCH)
#define SCREEN_SIZE_X   270
#define SCREEN_SIZE_Y   203
#define SCREEN_ASPECT   SCREEN_ASPECT_4_3
#else
#error "Unknown screen size"
#endif

// Screen timing
#if defined(SCREEN_600_800)
// 600x800 @ 60
#define SCREEN_CLK      61000
#define SCREEN_HACT     600
#define SCREEN_VACT     800
#define SCREEN_HBLK     600
#define SCREEN_HFP      64
#define SCREEN_HSYNC    192
#define SCREEN_VBLK     48
#define SCREEN_VFP      1
#define SCREEN_VSYNC    3
#elif defined(SCREEN_800_600)
// 800x600 @ 60, 40MHz DMT
#define SCREEN_CLK      40000
#define SCREEN_HACT     800
#define SCREEN_VACT     600
#define SCREEN_HBLK     256
#define SCREEN_HFP      40
#define SCREEN_HSYNC    128
#define SCREEN_VBLK     28
#define SCREEN_VFP      1
#define SCREEN_VSYNC    4
#elif defined(SCREEN_1024_758)
// 1024x758 @ 60, 62.5MHz CVT
#define SCREEN_CLK      62500
#define SCREEN_HACT     1024
#define SCREEN_VACT     758
#define SCREEN_HBLK     304
#define SCREEN_HFP      48
#define SCREEN_HSYNC    104
#define SCREEN_VBLK     29
#define SCREEN_VFP      3
#define SCREEN_VSYNC    10

#define TCON_HACT       256
#define TCON_HBP        2
#define TCON_HSYNC      2
#define TCON_HFP        72
#define TCON_VACT       758
#define TCON_VBP        3
#define TCON_VSYNC      1
#define TCON_VFP        12
#elif defined(SCREEN_1448_1072)
// 1448x1072 @ 60, 128.5MHz CVT
#define SCREEN_CLK      128500
#define SCREEN_HACT     1448
#define SCREEN_VACT     1072
#define SCREEN_HBLK     480
#define SCREEN_HFP      88
#define SCREEN_HSYNC    152
#define SCREEN_VBLK     40
#define SCREEN_VFP      3
#define SCREEN_VSYNC    10
#elif defined(SCREEN_1600_1200)
// 1600x1200 @ 60, 162MHz DMT
// #define SCREEN_CLK      162000
// #define SCREEN_HACT     1600
// #define SCREEN_VACT     1200
// #define SCREEN_HBLK     560
// #define SCREEN_HFP      64
// #define SCREEN_HSYNC    192
// #define SCREEN_VBLK     50
// #define SCREEN_VFP      1
// #define SCREEN_VSYNC    3
// 1600x1200 @ 60, 124.488MHz CVT-RB-v2
#define SCREEN_CLK      124488
#define SCREEN_HACT     1600
#define SCREEN_VACT     1200
#define SCREEN_HBLK     80
#define SCREEN_HFP      8
#define SCREEN_HSYNC    32
#define SCREEN_VBLK     35
#define SCREEN_VFP      21
#define SCREEN_VSYNC    8

#define TCON_HACT       400
#define TCON_HBP        2
#define TCON_HSYNC      2
#define TCON_HFP        16
#define TCON_VACT       1200
#define TCON_VBP        2
#define TCON_VSYNC      1
#define TCON_VFP        12
#elif defined(SCREEN_1872_1404)
// 1872x1404 @ 60, 162MHz Custom
#define SCREEN_CLK      162000
#define SCREEN_HACT     1872
#define SCREEN_VACT     1404
#define SCREEN_HBLK     38
#define SCREEN_HFP      6
#define SCREEN_HSYNC    10
#define SCREEN_VBLK     10
#define SCREEN_VFP      1
#define SCREEN_VSYNC    3
#elif defined(SCREEN_1920_1080)
// 1920x1080 @ 60, 148.5MHz CEA-861/DMT
#define SCREEN_CLK      148500
#define SCREEN_HACT     1920
#define SCREEN_VACT     1080
#define SCREEN_HBLK     280
#define SCREEN_HFP      88
#define SCREEN_HSYNC    44
#define SCREEN_VBLK     45
#define SCREEN_VFP      4
#define SCREEN_VSYNC    5
#elif defined(SCREEN_2200_1650)
// 2200x1650 @ 60, 224MHz Custom
#define SCREEN_CLK      224000
#define SCREEN_HACT     2200
#define SCREEN_VACT     1650
#define SCREEN_HBLK     36
#define SCREEN_HFP      6
#define SCREEN_HSYNC    10
#define SCREEN_VBLK     20
#define SCREEN_VFP      1
#define SCREEN_VSYNC    5
#endif
