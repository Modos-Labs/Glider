/*******************************************************************************
 * Freescale/NXP EPDC waveform firmware dumper
 * Based on https://github.com/julbouln/ice40_eink_controller
 *
 * This is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This software is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * the software. If not, see <http://www.gnu.org/licenses/>.
 * 
 * This file is partially derived from Linux kernel driver, with the following
 * copyright information:
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 ******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

struct waveform_data_header {
    unsigned int wi0;
    unsigned int wi1;
    unsigned int wi2;
    unsigned int wi3;
    unsigned int wi4;
    unsigned int wi5;
    unsigned int wi6;

    unsigned int xwia: 24;
    unsigned int cs1: 8;

    unsigned int wmta: 24;
    unsigned int fvsn: 8;
    unsigned int luts: 8;
    unsigned int mc: 8;
    unsigned int trc: 8;
    unsigned int advanced_wfm_flags: 8;
    unsigned int eb: 8;
    unsigned int sb: 8;
    unsigned int reserved0_1: 8;
    unsigned int reserved0_2: 8;
    unsigned int reserved0_3: 8;
    unsigned int reserved0_4: 8;
    unsigned int reserved0_5: 8;
    unsigned int cs2: 8;
};

struct mxcfb_waveform_data_file {
    struct waveform_data_header wdh;
    uint32_t *data;	/* Temperature Range Table + Waveform Data */
};

static uint64_t read_uint64_le(uint8_t* src) {
    return ((uint64_t)src[7] << 56) | 
            ((uint64_t)src[6] << 48) |
            ((uint64_t)src[5] << 40) |
            ((uint64_t)src[4] << 32) |
            ((uint64_t)src[3] << 24) |
            ((uint64_t)src[2] << 16) |
            ((uint64_t)src[1] << 8) |
            (uint64_t)src[0];
}

static uint8_t read_uint4(uint8_t* src, size_t addr) {
    uint8_t val = src[addr >> 1];
    if (addr & 1)
        val = (val >> 4) & 0xf;
    else
        val = val & 0xf;
    return val;
}

void dump_phases(FILE* fp, uint8_t* buffer, int phases, int ver) {
    int i, j, k, l, x, y;

    uint8_t luts[phases][16][16];

    k = 0;
    for (i = 0; i < phases * 256; i += 256) {
        j = 0;
        for (x = 0; x < 16; x++) {
            for (y = 0; y < 16; y++) {
                uint8_t val;
                if (ver == 2)
                    val = read_uint4(buffer + 8, i + j);
                else
                    val = buffer[8 + i + j];
                luts[k][y][x] = val;
                j++;
            }
        }
        k++;
    }

    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            fprintf(fp, "%d,%d,", i, j);
            for (k = 0; k < phases; k++) {
                fprintf(fp, "%d,", luts[k][i][j]);
            }
            fprintf(fp, "\n");

        }
    }
}

int main(int argc, char **argv) {
    fprintf(stderr, "MXC EPDC waveform dumper\n");

    if (argc < 4) {
        fprintf(stderr, "Usage: mxc_wvfm_dump version input_file output_prefix\n");
        fprintf(stderr, "version: EPDC version, possible values: v1, v2\n");
        fprintf(stderr, "input_file: MXC EPDC firmware file, in .fw format\n");
        fprintf(stderr, "output_prefix: Prefix for output file name, without extension\n");
        fprintf(stderr, "Example: mxc_wvfm_dump v1 epdc_E060SCM.fw e060scm\n");
        return 1;
    }

    char *ver_string = argv[1];
    char *fw = argv[2];
    char *prefix = argv[3];
    int ver;

    if (strcmp(ver_string, "v1") == 0) {
        ver = 1;
    }
    else if (strcmp(ver_string, "v2") == 0) {
        ver = 2;
    }
    else {
        fprintf(stderr, "Invalid EPDC version %s\n", ver_string);
        fprintf(stderr, "Possible values: v1, v2.\n");
        fprintf(stderr, "i.MX6DL/SL uses EPDCv1, i.MX7D uses EPDCv2. i.MX5 EPDC is not supported.\n");
        return 1;
    }

    FILE * fp;
    size_t file_size;
    char * file_buffer;
    size_t result;

    fp = fopen(fw, "rb");
    assert(fp);

    // obtain file size:
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // allocate memory to contain the whole file:
    file_buffer = (char*)malloc(sizeof(char) * file_size);
    assert(file_buffer);

    // copy the file into the buffer:
    assert(fread(file_buffer, file_size, 1, fp) == 1);

    fclose(fp);

    /* the whole file is now loaded in the memory buffer. */

    struct mxcfb_waveform_data_file *wv_file;
    wv_file = (struct mxcfb_waveform_data_file *)file_buffer;

    printf("wi0: %08x\n", wv_file->wdh.wi0);
    printf("wi1: %08x\n", wv_file->wdh.wi1);
    printf("wi2: %08x\n", wv_file->wdh.wi2);
    printf("wi3: %08x\n", wv_file->wdh.wi3);
    printf("wi4: %08x\n", wv_file->wdh.wi4);
    printf("wi5: %08x\n", wv_file->wdh.wi5);
    printf("wi6: %08x\n", wv_file->wdh.wi6);

    printf("xwia: %d\n", wv_file->wdh.xwia);
    printf("cs1: %d\n", wv_file->wdh.cs1);

    printf("wmta:  %d\n", wv_file->wdh.wmta);
    printf("fvsn: %d\n", wv_file->wdh.fvsn);
    printf("luts: %d\n", wv_file->wdh.luts);
    printf("mc: %d\n", wv_file->wdh.mc);
    printf("trc: %d\n", wv_file->wdh.trc);
    printf("advanced_wfm_flags: %d\n", wv_file->wdh.advanced_wfm_flags);
    printf("eb: %d\n", wv_file->wdh.eb);
    printf("sb: %d\n", wv_file->wdh.sb);
    printf("reserved0_1: %d\n", wv_file->wdh.reserved0_1);
    printf("reserved0_2: %d\n", wv_file->wdh.reserved0_2);
    printf("reserved0_3: %d\n", wv_file->wdh.reserved0_3);
    printf("reserved0_4: %d\n", wv_file->wdh.reserved0_4);
    printf("reserved0_5: %d\n", wv_file->wdh.reserved0_5);
    printf("cs2: %d\n", wv_file->wdh.cs2);

    int i, j;
    int trt_entries; //  temperature range table
    int wv_data_offs; //  offset for waveform data
    int waveform_buffer_size; // size for waveform data
    int mode_count = wv_file->wdh.mc + 1;

    uint8_t *temp_range_bounds;
    trt_entries = wv_file->wdh.trc + 1;

    printf("Temperatures count: %d\n", trt_entries);

    temp_range_bounds = (uint8_t*)malloc(trt_entries);

    memcpy(temp_range_bounds, &wv_file->data, trt_entries);

    for (i = 0; i < trt_entries; i++) {
        printf("Temperature %d = %d°C\n", i, temp_range_bounds[i]);
    }

    wv_data_offs = sizeof(wv_file->wdh) + trt_entries + 1;
    waveform_buffer_size = file_size - wv_data_offs;

    printf("Waveform data offset: %d, size: %d\n", wv_data_offs, waveform_buffer_size);

    if ((wv_file->wdh.luts & 0xC) == 0x4) {
        printf("waveform 5bit\n");
    } else {
        printf("waveform 4bit\n");
    }

    uint8_t	*waveform_buffer;
    waveform_buffer = (uint8_t *)malloc(waveform_buffer_size);
    memcpy(waveform_buffer, (uint8_t *)(file_buffer) + wv_data_offs,
            waveform_buffer_size);

    uint64_t* wv_modes = malloc(sizeof(uint64_t) * mode_count);

    uint64_t addr;
    // get modes addr
    for (i = 0; i < mode_count; i++) {
        addr = read_uint64_le(&waveform_buffer[i * 8]);
        printf("wave #%d addr: %08"PRIx64"\n", i, addr);
        wv_modes[i] = addr;
    }

    // get modes temp addr
    uint64_t* wv_modes_temps = malloc(sizeof(uint64_t) * mode_count * trt_entries);
    uint64_t* frame_counts = malloc(sizeof(uint64_t) * mode_count * trt_entries);
    uint64_t last_addr = wv_modes[0];
    for (i = 0; i < mode_count; i++) {
        uint64_t m = wv_modes[i];
        for (j = 0; j < trt_entries; j++) {
            addr = read_uint64_le(&waveform_buffer[m + j * 8]);
            wv_modes_temps[i * trt_entries + j] = addr;
            uint64_t frame_count = read_uint64_le(&waveform_buffer[addr]);
            frame_counts[i * trt_entries + j] = frame_count;
            printf("wave #%d, temp #%d addr: %08"PRIx64", %"PRId64" phases (Addr diff = %"PRId64", Size = %"PRId64")\n", i, j,
                    addr, frame_count, addr - last_addr, frame_count * 256);
            last_addr = addr;
        }
    }

    char* fn = malloc(strlen(prefix) + 14);
    sprintf(fn, "%s_desc.iwf", prefix);
    fp = fopen(fn, "w");
    assert(fp);
    fprintf(fp, "[WAVEFORM]\n");
    fprintf(fp, "VERSION = 1.0\n");
    fprintf(fp, "PREFIX = %s\n", prefix);
    fprintf(fp, "MODES = %d\n", mode_count);
    fprintf(fp, "TEMPS = %d\n", trt_entries);
    fprintf(fp, "\n");
    for (int i = 0; i < trt_entries; i++) {
        fprintf(fp, "T%dRANGE = %d\n", i, temp_range_bounds[i]);
    }
    fprintf(fp, "\n");
    for (int i = 0; i < mode_count; i++) {
        fprintf(fp, "[MODE%d]\n", i);
        for (int j = 0; j < trt_entries; j++) {
            fprintf(fp, "T%dFC = %"PRId64"\n", j, frame_counts[i * trt_entries + j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);

    for (int i = 0; i < mode_count; i++) {
        for (int j = 0; j < trt_entries; j++) {
            sprintf(fn, "%s_M%d_T%d.csv", prefix, i, j);
            fp = fopen(fn, "w");
            assert(fp);
            size_t index = i * trt_entries + j;
            uint64_t frame_count = frame_counts[index];
            uint8_t* lut = &waveform_buffer[wv_modes_temps[index]];
            dump_phases(fp, lut, frame_count, ver);
            fclose(fp);
        }
    }

    free(fn);
    free(temp_range_bounds);
    free(waveform_buffer);
    free(frame_counts);
    free(wv_modes);
    free(wv_modes_temps);

    free(file_buffer);

    return 0;
}