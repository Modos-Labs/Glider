/*******************************************************************************
 * Freescale/NXP i.MX EPDC waveform assembler
 * 
 * This tools converts human readable .csv waveform file into .fw file used
 * by i.MX EPDC driver.
 * 
 * Copyright 2021 Wenting Zhang
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
 * 
 * inih is used in this project, which is licensed under BSD-3-Clause.
 * csv_parser is used in this project, which is licensed under MIT.
 ******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>
#include <inttypes.h>
#include "ini.h"
#include "csv.h"

#define MAX_MODES (32) // Maximum waveform modes supported
#define MAX_TEMPS (32) // Maximum temperature ranges supported

#define MAX_CSV_LINE (1024)

#define GREYSCALE_BPP   (2)
#define GREYSCALE_LEVEL (16)

typedef struct {
    unsigned int wi0;
    unsigned int wi1;
    unsigned int wi2;
    unsigned int wi3;
    unsigned int wi4;
    unsigned int wi5;
    unsigned int wi6;

    unsigned int xwia: 24; // address of extra waveform information
    unsigned int cs1: 8; // checksum 1

    unsigned int wmta: 24;
    unsigned int fvsn: 8;
    unsigned int luts: 8;
    unsigned int mc: 8; // mode count
    unsigned int trc: 8; // temperature range count
    unsigned int advanced_wfm_flags: 8;
    unsigned int eb: 8;
    unsigned int sb: 8;
    unsigned int reserved0_1: 8;
    unsigned int reserved0_2: 8;
    unsigned int reserved0_3: 8;
    unsigned int reserved0_4: 8;
    unsigned int reserved0_5: 8;
    unsigned int cs2: 8; // checksum 2
} waveform_data_header_t;

typedef struct {
    waveform_data_header_t wdh;
    uint8_t data[];    /* Temperature Range Table + Waveform Data */
} waveform_data_file_t;

typedef struct {
    char *prefix;
    int modes;
    char **mode_names;
    int *frame_counts; // frame_counts[mode * temps + temp]
    int temps;
    int *temp_ranges;
    uint8_t ***luts; // luts[mode][temp][frame count * 256 + dst * 16 + src]
} context_t;

static int ini_parser_handler(void* user, const char* section, const char* name,
        const char* value) {
    context_t* pcontext = (context_t*)user;

    if (strcmp(section, "WAVEFORM") == 0) {
        if (strcmp(name, "VERSION") == 0) {
            assert(strcmp(value, "1.0") == 0);
        }
        else if (strcmp(name, "PREFIX") == 0) {
            pcontext->prefix = strdup(value);
        }
        else if (strcmp(name, "MODES") == 0) {
            // Allocate memory for modes
            pcontext->modes = atoi(value);
            assert(pcontext->modes <= MAX_MODES);
            pcontext->mode_names = malloc(sizeof(char*) * pcontext->modes);
            assert(pcontext->mode_names);
        }
        else if (strcmp(name, "TEMPS") == 0) {
            // Allocate memory for temp ranges
            pcontext->temps = atoi(value);
            assert(pcontext->temps <= MAX_TEMPS);
            pcontext->temp_ranges = malloc(sizeof(int) * pcontext->temps);
            assert(pcontext->temp_ranges);
            pcontext->frame_counts = malloc(sizeof(int) * pcontext->modes *
                    pcontext->temps);
            assert(pcontext->frame_counts);
        }
        else {
            size_t len = strlen(name);
            if ((len >= 7) && (name[0] == 'T') &&
                    (strncmp(name + (len - 5), "RANGE", 5) == 0)) {
                // Temperature Range
                char *temp_id_s = strdup(name);
                temp_id_s[len - 5] = '\0';
                int temp_id = atoi(temp_id_s + 1);
                free(temp_id_s);
                pcontext->temp_ranges[temp_id] = atoi(value);
            }
            else {
                fprintf(stderr, "Unknown name %s=%s\n", name, value);
                return 0; // Unknown name
            }
        }
    }
    else if (strncmp(section, "MODE", 4) == 0) {
        int mode_id = atoi(section + 4);
        assert(mode_id >= 0);
        assert(mode_id < pcontext->modes);
        size_t len = strlen(name);
        if (strcmp(name, "NAME") == 0) {
            // Mode Name
            pcontext->mode_names[mode_id] = strdup(value);
        }
        else if ((len >= 4) && (name[0] == 'T') &&
                (strncmp(name + (len - 2), "FC", 2) == 0)) {
            // Frame Count
            char *temp_id_s = strdup(name);
            temp_id_s[len - 2] = '\0';
            int temp_id = atoi(temp_id_s + 1);
            free(temp_id_s);
            pcontext->frame_counts[pcontext->temps * mode_id + temp_id]
                    = atoi(value);
        }
    }
    else {
        fprintf(stderr, "Unknown section %s\n", section);
        return 0; // Unknown section
    }
    return 1;
}

static void write_uint64_le(uint8_t* dst, uint64_t val) {
    dst[7] = (val >> 56) & 0xff;
    dst[6] = (val >> 48) & 0xff;
    dst[5] = (val >> 40) & 0xff;
    dst[4] = (val >> 32) & 0xff;
    dst[3] = (val >> 24) & 0xff;
    dst[2] = (val >> 16) & 0xff;
    dst[1] = (val >> 8) & 0xff;
    dst[0] = (val) & 0xff;
}

static void parse_range(const char* str, int* begin, int* end) {
    // Parse range specified in the waveform.
    // Example:
    // 2 - 2 to 2
    // 0:15 - 0 to 15
    // 4:7 - 4 to 7
    char* delim = strchr(str, ':');
    if (delim) {
        *begin = atoi(str);
        *end = atoi(delim + 1);
    }
    else {
        *begin = atoi(str);
        *end = *begin;
    }
}

static void load_waveform_csv(const char* filename, int frame_count,
        uint8_t* lut) {
    FILE* fp = fopen(filename, "r");
    assert(fp);

    // Unspecified parts of LUT will be filled with 3 instead of 0 for debugging
    memset(lut, 3, frame_count * 256);

    char* line;
    int done = 0;
    int err = 0;
    int rst = 1; // Reset fread_csv_line internal state in the first call
    while (!done) {
        line = fread_csv_line(fp, MAX_CSV_LINE, &done, &err, rst);
        rst = 0;
        if (!line) continue;
        char** parsed = parse_csv(line);
        if (!parsed) continue;
        // Parse source/ destination range
        int src0, src1, dst0, dst1;
        // Skip empty lines
        if (!parsed[0]) continue;
        if (!parsed[1]) continue;
        parse_range(parsed[0], &src0, &src1);
        parse_range(parsed[1], &dst0, &dst1);
        // Fill in LUT
        for (int i = 0; i < frame_count; i++) {
            assert(parsed[i]);
            uint8_t val = atoi(parsed[i + 2]);
            for (int src = src0; src <= src1; src++) {
                for (int dst = dst0; dst <= dst1; dst++) {
                    lut[i * 256 + dst * 16 + src] = val;
                }
            }
        }
        free_csv_line(parsed);
        free(line);
    }

    fclose(fp);
}

static void dump_lut(int frame_count, uint8_t* lut) {
    for (int src = 0; src < 16; src++) {
        for (int dst = 0; dst < 16; dst++) {
            printf("%x -> %x: ", src, dst);
            for (int frame = 0; frame < frame_count; frame++) {
                printf("%d ", lut[frame * 256 + dst * 16 + src]);
            }
            printf("\n");
        }
    }
}

static void copy_lut(uint8_t* dst, uint8_t* src, size_t src_count, int ver) {
    if (ver == 1) {
        memcpy(dst, src, src_count);
    }
    else {
        for (size_t i = 0; i < src_count / 2; i++) {
            uint8_t val;
            val = *src++;
            val <<= 4;
            val |= *src++;
            *dst++ = val;
        }
    }
}

int main(int argc, char *argv[]) {
    context_t context;
    
    printf("Freescale/NXP i.MX EPDC waveform assembler\n");

    // Load waveform descriptor
    if (argc < 4) {
        fprintf(stderr, "Usage: mxc_wvfm_asm version input_file output_file\n");
        fprintf(stderr, "version: EPDC version, possible values: v1, v2\n");
        fprintf(stderr, "input_file: Waveform file, in .iwf format\n");
        fprintf(stderr, "output_file: MXC EPDC firmware file, in .fw format\n");
        fprintf(stderr, "Example: mxc_wvfm_asm v1 e060scm_desc.iwf epdc_E060SCM.fw\n");
        return 1;
    }

    char* ver_string = argv[1];
    char* input_fn = argv[2];
    char* output_fn = argv[3];
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
        fprintf(stderr, "i.MX6DL/SL uses EPDCv1, i.MX7D uses EPDCv2. "
                "i.MX5 EPDC is not supported.\n");
        return 1;
    }

    if (ini_parse(input_fn, ini_parser_handler, &context) < 0) {
        fprintf(stderr, "Failed to load waveform descriptor.\n");
        return 1;
    }

    // Set default name if not provided
    char* default_name = "Unknown";
    for (int i = 0; i < context.modes; i++) {
        if (!context.mode_names[i])
            context.mode_names[i] = strdup(default_name);
    }

    // Print loaded info
    printf("Prefix: %s\n", context.prefix);

    for (int i = 0; i < context.modes; i++) {
        printf("Mode %d: %s\n", i, context.mode_names[i]);
        for (int j = 0; j < context.temps; j++) {
            printf("\tTemp %d: %d frames\n", j,
                    context.frame_counts[i * context.temps + j]);
        }
    }

    for (int i = 0; i < context.temps; i++) {
        printf("Temp %d: %d degC\n", i, context.temp_ranges[i]);
    }

    assert(context.modes < 100);
    assert(context.temps < 100);

    // Load actual waveform
    char *dir = dirname(input_fn); // Return val of dirname shall not be free()d
    size_t dirlen = strlen(dir);
    context.luts = malloc(context.modes * sizeof(uint8_t**));
    assert(context.luts);
    for (int i = 0; i < context.modes; i++) {
        context.luts[i] = malloc(context.temps * sizeof(uint8_t*));
        assert(context.luts[i]);
        for (int j = 0; j < context.temps; j++) {
            int frame_count = context.frame_counts[i * context.temps + j];
            context.luts[i][j] = malloc(frame_count * 256); // LUT always in 8b
            assert(context.luts[i][j]);
            char* fn = malloc(dirlen + strlen(context.prefix) + 14);
            assert(fn);
            sprintf(fn, "%s/%s_M%d_T%d.csv", dir, context.prefix, i, j);
            printf("Loading %s...\n", fn);
            load_waveform_csv(fn, frame_count, context.luts[i][j]);
            free(fn);
        }
    }

    // Calculate file size and offset
    uint64_t header_size = sizeof(waveform_data_header_t);
    uint64_t temp_table_size = sizeof(uint8_t) * context.temps;
    uint64_t mode_offset_table_size = sizeof(uint64_t) * context.modes;
    uint64_t temp_offset_table_size = sizeof(uint64_t) * context.temps;

    // 1st level mode offset table
    uint64_t* mode_offset_table = malloc(mode_offset_table_size);
    // global offset table
    uint64_t* data_offset_table =
            malloc(sizeof(uint64_t) * context.modes * context.temps);
    uint64_t total_size = 0;
    uint64_t data_region_offset = temp_table_size + 1;
    total_size += mode_offset_table_size;
    uint64_t lut_size = (ver == 1) ? 256 : 128;

    for (int i = 0; i < context.modes; i++) {
        // Set the offset of the current mode
        mode_offset_table[i] = total_size;
        total_size += temp_offset_table_size;
        for (int j = 0; j < context.temps; j++) {
            uint64_t data_size = context.frame_counts[i * context.temps + j]
                    * lut_size + sizeof(uint64_t);
            data_offset_table[i * context.temps + j] = total_size;
            printf("Mode %d Temp %d data offset %08"PRIx64" (%"PRId64")\n",
                    i, j, total_size, total_size);
            total_size += data_size;
        }
    }
    total_size += header_size + temp_table_size + 1;
    printf("Total file size %"PRId64"\n", total_size);

    // Allocate memory for waveform buffer
    waveform_data_file_t* pwvfm_file = malloc(total_size);
    assert(pwvfm_file);

    // Fill waveform header
    memset(&pwvfm_file->wdh, 0, sizeof(waveform_data_header_t));
    pwvfm_file->wdh.trc = context.temps - 1;
    pwvfm_file->wdh.mc = context.modes - 1;
    // Other fields (including checksums) are generally directly imported from
    // wbf file. They are not used in MXC EPDC driver. No need to fill them.

    // Fill temperature table
    for (int i = 0; i < context.temps; i++) {
        pwvfm_file->data[i] = context.temp_ranges[i];
    }

    // Set 1 byte padding
    pwvfm_file->data[context.temps] = 0;

    // Fill waveform offset table and temp offset table
    uint8_t* wvfm_data_region = &pwvfm_file->data[data_region_offset];
    for (int i = 0; i < context.modes; i++) {
        write_uint64_le(&wvfm_data_region[i * 8],
                mode_offset_table[i]);
        for (int j = 0; j < context.temps; j++) {
            write_uint64_le(&wvfm_data_region[mode_offset_table[i] + j * 8],
                    data_offset_table[i * context.temps + j]);
        }
    }

    // Fill waveform data
    for (int i = 0; i < context.modes; i++) {
        for (int j = 0; j < context.temps; j++) {
            size_t index = i * context.temps + j;
            uint8_t* wvfm_wr_ptr = &wvfm_data_region[data_offset_table[index]];
            int frame_count = context.frame_counts[index];
            write_uint64_le(wvfm_wr_ptr, frame_count);
            wvfm_wr_ptr += 8;
            copy_lut(wvfm_wr_ptr, context.luts[i][j], frame_count * 256, ver);
        }
    }

    // Write waveform file
    FILE *outFile = fopen(output_fn, "wb");
    assert(outFile);

    size_t written = fwrite((uint8_t *)pwvfm_file, total_size, 1, outFile);
    assert(written == 1);

    fclose(outFile);

    printf("Finished.\n");

    // Free buffers
    free(pwvfm_file);
    free(context.prefix);
    for (int i = 0; i < context.modes; i++) {
        free(context.mode_names[i]);
        for (int j = 0; j < context.temps; j++) {
            free(context.luts[i][j]);
        }
        free(context.luts[i]);
    }
    free(context.mode_names);
    free(context.luts);
    free(context.frame_counts);
    free(context.temp_ranges);
    free(mode_offset_table);
    free(data_offset_table);

    return 0;
}
