/*******************************************************************************
 * Eink waveform firmware dumper
 * Based on https://github.com/fread-ink/inkwave and Linux kernel
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
 * Copyright 2004-2013 Freescale Semiconductor, Inc.
 * Copyright 2005-2017 Amazon Technologies, Inc.
 * Copyright 2018, 2021 Marc Juul
 * Copyright (C) 2022 Samuel Holland <samuel@sholland.org>
 * Copyright 2024 Wenting Zhang
 ******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>
#include <ctype.h>

struct waveform_data_header {
  uint32_t checksum:32; // 0
  uint32_t filesize:32; // 4
  uint32_t serial:32; // 8 serial number
  uint32_t run_type:8; // 12
  uint32_t fpl_platform:8; // 13
  uint32_t fpl_lot:16; // 14
  uint32_t mode_version_or_adhesive_run_num:8; // 16
  uint32_t waveform_version:8; // 17
  uint32_t waveform_subversion:8; // 18
  uint32_t waveform_type:8; // 19
  uint32_t fpl_size:8; // 20 (aka panel_size)
  uint32_t mfg_code:8; // 21 (aka amepd_part_number)
  uint32_t waveform_tuning_bias_or_rev:8; // 22
  uint32_t fpl_rate:8; // 23 (aka frame_rate)
  uint32_t unknown0:8;
  uint32_t vcom_shifted:8;
  uint32_t unknown1:16;
  uint32_t xwia:24; // address of extra waveform information
  uint32_t cs1:8; // checksum 1
  uint32_t wmta:24;
  uint32_t fvsn:8;
  uint32_t luts:8;
  uint32_t mc:8; // mode count (length of mode table - 1)
  uint32_t trc:8; // temperature range count (length of temperature table - 1)
  uint32_t advanced_wfm_flags:8;
  uint32_t eb:8;
  uint32_t sb:8;
  uint32_t reserved0_1:8;
  uint32_t reserved0_2:8;
  uint32_t reserved0_3:8;
  uint32_t reserved0_4:8;
  uint32_t reserved0_5:8;
  uint32_t cs2:8; // checksum 2
}__attribute__((packed));

#define HEADER_SIZE sizeof(struct waveform_data_header)

#define MODE_MAX 10

// Mode version to mode string
struct mode_name_lut_t {
    uint8_t versions[2];
    char *mode_strings[MODE_MAX];
};

// Partially derived from linux kernel drm_epd_helper.c
// All GL series (GL GLR GLD) are marked as GL as the underlying wavetable seems to be identical
// Thus it's impossible to identify the correct ordering
// -R/ -D ghosting reduction is outside of the scope of this tool
const struct mode_name_lut_t mode_name_lut[] = {
    {
        // Example: ED050SC3
        .versions = {0x01},
        .mode_strings = {"INIT", "DU", "GL8", "GC8"}
    },
    {
        // Example: ED097TC1
        .versions = {0x03},
        .mode_strings = {"INIT", "DU", "GL16", "GL16", "A2"}
    },
    {
        // Example: ET073TC1
        .versions = {0x09},
        .mode_strings = {"INIT", "DU", "GC16", "A2"}
    },
    {
        // Untested
        .versions = {0x12},
        .mode_strings = {"INIT", "DU", "NULL", "GC16", "A2", "GL16", "GL16", "DU4"}
    },
    {
        // Example: ES133UT1
        .versions = {0x15},
        .mode_strings = {"INIT", "DU", "GC16", "GL16", "A2", "DU4", "GC4"}
    },
    {
        // Untested
        .versions = {0x16},
        .mode_strings = {"INIT", "DU", "GC16", "GL16", "GL16", "GC16", "A2"}
    },
    {
        // Example: ES108FC1
        .versions = {0x18, 0x20},
        .mode_strings = {"INIT", "DU", "GC16", "GL16", "GL16", "GL16", "A2"}
    },
    {
        // Example: ES103TC1
        .versions = {0x19, 0x43},
        .mode_strings = {"INIT", "DU", "GC16", "GL16", "GL16", "GL16", "A2", "DU4"}
    },
    {
        // Untested
        .versions = {0x23},
        .mode_strings = {"INIT", "DU", "GC16", "GL16", "A2", "DU4"}
    },
    {
        // Untested
        .versions = {0x54},
        .mode_strings = {"INIT", "DU", "GC16", "GL16", "GL16", "A2"}
    },
    {
        // Example: ES120MC1
        .versions = {0x48},
        .mode_strings = {"INIT", "DU", "GC16", "GL16", "GL16", "NULL", "A2"}
    }
};
#define MODE_NAME_LUTS  (sizeof(mode_name_lut) / sizeof(*mode_name_lut))

#define MAX_TABLE_LENGTH    (1024*1024)

static uint32_t read_pointer(uint8_t *ptr) {
    uint32_t addr = ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[1] << 8) | ((uint32_t)ptr[0]);
    uint8_t checksum = ptr[0] + ptr[1] + ptr[2];
    if (checksum != ptr[3]) {
        printf("Pointer checksum mismatch\n");
    }
    return addr;
}

static uint8_t read_uint4(uint8_t* src, size_t addr) {
    uint8_t val = src[addr >> 1];
    if (addr & 1)
        val = (val >> 4) & 0xf;
    else
        val = val & 0xf;
    return val;
}


void compute_crc_table(unsigned int* crc_table) {
   unsigned c;
   int n, k;
   for (n = 0; n < 256; n++) {
      c = (unsigned) n;
      for (k = 0; k < 8; k++) {
         if (c & 1) {
            c = 0xedb88320L ^ (c >> 1);
         }
         else {
            c = c >> 1;
         }
      }
      crc_table[n] = c;
   }
}

unsigned int update_crc(unsigned int* crc_table, unsigned crc,
                           unsigned char *buf, int len) {

  char b;
  unsigned c = crc ^ 0xffffffff;
  int i;
  
  for(i=0; i < len; i++) {
    if(!buf) {
      b = 0;
    } else {
      b = buf[i];
    }
    c = crc_table[(c ^ b) & 0xff] ^ (c >> 8);
  }
  
  return c ^ 0xffffffff;
}

unsigned crc32(unsigned char *buf, int len) {
  static unsigned int crc_table[256];
  
  compute_crc_table(crc_table);
  
  return update_crc(crc_table, 0, buf, len);
}

void dump_phases(FILE* fp, uint8_t* buffer, int bpp, int phases, int phase_per_byte) {
    int states;
    int i, j, k, x, y;

    states = (bpp == 4) ? 16 : 32;
    uint8_t luts[phases][states][states];
    uint8_t *ptr = buffer;

    for (i = 0; i < phases; i ++) {
        for (x = 0; x < states; x++) {
            for (y = 0; y < states; y+=phase_per_byte) {
                uint8_t val = *ptr++;
                if (phase_per_byte == 4) {
                    luts[i][y+0][x] = (val >> 0) & 0x3;
                    luts[i][y+1][x] = (val >> 2) & 0x3;
                    luts[i][y+2][x] = (val >> 4) & 0x3;
                    luts[i][y+3][x] = (val >> 6) & 0x3;
                }
                else if (phase_per_byte == 2) {
                    luts[i][y+0][x] = (val >> 0) & 0xf;
                    luts[i][y+1][x] = (val >> 4) & 0xf;
                }
            }
        }
    }

    for (i = 0; i < states; i++) {
        for (j = 0; j < states; j++) {
            fprintf(fp, "%d,%d,", i, j);
            for (k = 0; k < phases; k++) {
                fprintf(fp, "%d,", luts[k][i][j]);
            }
            fprintf(fp, "\n");
        }
    }
}

int main(int argc, char **argv) {
    fprintf(stderr, "Eink wbf waveform dumper\n");

    if (argc != 3) {
        fprintf(stderr, "Usage: wbf_wvfm_dump input_file output_prefix\n");
        fprintf(stderr, "input_file: MXC EPDC firmware file, in .wbf format\n");
        fprintf(stderr, "output_prefix: Prefix for output file name, without extension\n");
        fprintf(stderr, "Example: wbf_wvfm_dump E060SCM.wbf e060scm\n");
        return 1;
    }

    char *fw = argv[1];
    char *prefix = argv[2];

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

    struct waveform_data_header *header;
    header = (struct waveform_data_header *)file_buffer;

    printf("File size: %zu bytes.\n", file_size);

    printf("Reported size: %d bytes.\n", header->filesize);
    printf("Serial number: %d\n", header->serial);
    printf("Run type: 0x%x\n", header->run_type);
    printf("Mfg code: 0x%x\n", header->mfg_code);

    printf("FPL platform: 0x%x\n", header->fpl_platform);
    printf("FPL lot: 0x%x\n", header->fpl_lot);
    printf("FPL size: 0x%x\n", header->fpl_size);
    printf("FPL rate: 0x%x\n", header->fpl_rate);

    printf("Mode version: 0x%x\n", header->mode_version_or_adhesive_run_num);
    printf("Waveform version: %d\n", header->waveform_version);
    printf("Waveform sub-version: %d\n", header->waveform_subversion);
    
    printf("Waveform type: %d\n", header->waveform_type);

    printf("Number of modes: %d\n", header->mc + 1);
    printf("Number of temperature ranges: %d\n", header->trc + 1);

    int bpp = ((header->luts & 0xC) == 0x4) ? 5: 4;
    printf("BPP: %d (LUTS = 0x%02x)\n", bpp, header->luts);
    bool acep = false;
    if (header->luts == 0x15) {
        printf("Looks like you've supplied a waveform for ACeP screens\n");
        printf("Expect the checksum for the header to fail.\n");
        bpp = 5;
        acep = true;
    }

    // Compare checksum
    static unsigned int crc_table[256];
    compute_crc_table(crc_table);
    unsigned int crc;
    if (header->filesize != 0) {
        crc = update_crc(crc_table, 0, NULL, 4);
        crc = update_crc(crc_table, crc, (unsigned char *)file_buffer + 4, header->filesize-4);

        printf("Checksum: 0x%08x\n", crc);
        if (crc == header->checksum) {
            printf("Checksum match.\n");
        } else {
            printf("Checksum mismatch! Expected: 0x%08x\n", header->checksum);
        }
    }
    else {
        printf("File size reported to be 0 in the header. Checksum check skipped.\n");
    }

    // Try to find mode description
    char **mode_string = NULL;
    uint8_t mode_version = header->mode_version_or_adhesive_run_num;
    for (int i = 0; i < MODE_NAME_LUTS; i++) {
        if ((mode_name_lut[i].versions[0] == mode_version) ||
                (mode_name_lut[i].versions[1] == mode_version)) {
            mode_string = (char **)(mode_name_lut[i].mode_strings);
        }
    }
    if (mode_string) {
        printf("Known mode version, mode names available.\n");
    }
    else {
        printf("Unknown mode version, mode names won't be available.\n");
    }

    // Right following the header is the temperature range table
    uint8_t checksum = 0;

    int i, j;
    int trt_entries; //  temperature range table

    uint8_t *temp_range_bounds;
    trt_entries = header->trc + 1;

    printf("Temperatures count: %d\n", trt_entries);

    temp_range_bounds = (uint8_t*)malloc(trt_entries + 2);

    memcpy(temp_range_bounds, file_buffer + HEADER_SIZE, trt_entries + 2);

    for (i = 0; i < trt_entries; i++) {
        printf("Temperature %d = %d°C\n", i, temp_range_bounds[i]);
        checksum += temp_range_bounds[i];
    }

    printf("End bound: %d°C\n", temp_range_bounds[trt_entries]);
    checksum +=  temp_range_bounds[trt_entries];

    if (checksum != temp_range_bounds[trt_entries + 1]) {
        printf("Temperature table checksum mismatch\n");
    }

    char xwia_buffer[128];
    uint8_t xwia_len = 0;
    if (header->xwia != 0) {
        printf("Extra waveform information present:\n");
        uint8_t *ptr = (uint8_t *)file_buffer + header->xwia;
        xwia_len = *ptr++;
        checksum = xwia_len;

        j = 0;
        for (i = 0; i < xwia_len; i++) {
            char c = *ptr++;
            if (isprint(c)) {
                xwia_buffer[j++] = c;
            }
            else {
                xwia_buffer[j++] = '\\';
                xwia_buffer[j++] = 'x';
                xwia_buffer[j++] = (c / 16) + '0';
                xwia_buffer[j++] = (c % 16) + '0';
            }
            checksum += c;
        }
        xwia_buffer[j++] = '\0';
        printf("%s", xwia_buffer);
        printf("\n");
        if (checksum != *ptr++) {
            printf("XWIA checksum mismatch\n");
        }
    }

    int wv_data_offs; //  offset for waveform data
    int waveform_buffer_size; // size for waveform data
    int mode_count = header->mc + 1;

    wv_data_offs = HEADER_SIZE + trt_entries + 2 + 1 + xwia_len + 1;
    // This should yield the same result
    if ((header->xwia) && (wv_data_offs != header->xwia + 1 + xwia_len + 1))
        printf("Warning: data offset calculation mismatch\n");
    printf("Waveform data offset: %d\n", wv_data_offs);
    
    uint8_t *ptr = (uint8_t *)file_buffer + wv_data_offs;

    uint32_t* wv_modes = malloc(sizeof(uint32_t) * mode_count);

    uint32_t addr;
    // get modes addr
    for (i = 0; i < mode_count; i++) {
        addr = read_pointer(ptr);
        printf("wave #%d addr: %u\n", i, addr);
        wv_modes[i] = addr;
        ptr += 4;
    }

    // get modes temp addr
    // Table offsets, ordered as first shown in the wbf, may not use up all space
    uint32_t *table_offsets = malloc(sizeof(uint32_t) * mode_count * trt_entries + 1);
    int *frame_counts = malloc(sizeof(uint32_t) * mode_count * trt_entries);
    int tables = 0;
    // Table index for each mode x temp
    int *wv_modes_temps = malloc(sizeof(int) * mode_count * trt_entries);

    for (i = 0; i < mode_count; i++) {
        ptr = (uint8_t *)file_buffer + wv_modes[i];
        for (j = 0; j < trt_entries; j++) {
            addr = read_pointer(ptr);

            // Find the table ID correspond to the address
            // Ideally this should be an hash table, but given the extremely small
            // problem size here, linear search is more than good enough.
            int id = -1;
            for (int k = 0; k < tables; k++)
                if (table_offsets[k] == addr) {
                    id = k;
                    break;
                }

            if (id == -1) {
                // New table
                id = tables; // Assign ID
                tables++;
                table_offsets[id] = addr;
            }

            wv_modes_temps[i * trt_entries + j] = id;
            printf("wave #%d, temp #%d: wavetable %d\n", i, j, id);
            ptr += 4;
        }
    }

    char* fn = malloc(strlen(prefix) + 14);

    static uint8_t derle_buffer[MAX_TABLE_LENGTH];

    for (i = 0; i < tables; i++) {
        printf("Parsing table %d, addr %d (0x%06x)\n", i, table_offsets[i], table_offsets[i]);

        ptr = (uint8_t *)file_buffer + table_offsets[i];
        // Parse table
        bool rle_mode = true;
        uint32_t idx = 0;
        uint8_t checksum = 0;
        while(1) {
            uint8_t chr = *ptr++;
            checksum += chr;
            if (chr == 0xfc) {
                // Toggle RLE mode
                rle_mode = !rle_mode;
            }
            else if (chr == 0xff) {
                // End of block
                break;
            }
            else if (!rle_mode) {
                derle_buffer[idx++] = chr;
            }
            else {
                uint8_t len = *ptr++;
                checksum += len;
                for (j = 0; j < (int)len + 1; j++) {
                    derle_buffer[idx++] = chr;
                }
            }
        }
        printf("Total %d bytes. Checksum: 0x%02x\n", idx, checksum);
        if (*ptr++ != checksum) {
            printf("Checksum mismatch!\n");
        }

        int phase_per_byte = acep ? 2 : 4;
        int transitions = ((bpp == 4) ? (16 * 16) : (32 * 32));
        int phases = idx * phase_per_byte / transitions;
        frame_counts[i] = phases;

        // Dump phases
        sprintf(fn, "%s_TB%d.csv", prefix, i);
        fp = fopen(fn, "w");
        assert(fp);
        size_t index = i * trt_entries + j;
        dump_phases(fp, derle_buffer, bpp, phases, phase_per_byte);
        fclose(fp);
    }

    sprintf(fn, "%s_desc.iwf", prefix);
    fp = fopen(fn, "w");
    assert(fp);
    fprintf(fp, "[WAVEFORM]\n");
    fprintf(fp, "VERSION = 2.0\n");
    fprintf(fp, "PREFIX = %s\n", prefix);
    fprintf(fp, "NAME = %s\n", xwia_buffer);
    fprintf(fp, "BPP = %d\n", bpp);
    fprintf(fp, "MODES = %d\n", mode_count);
    fprintf(fp, "TEMPS = %d\n", trt_entries);
    fprintf(fp, "TABLES = %d\n", tables);
    fprintf(fp, "\n");
    for (int i = 0; i < trt_entries; i++) {
        fprintf(fp, "T%dRANGE = %d\n", i, temp_range_bounds[i]);
    }
    fprintf(fp, "TUPBOUND = %d\n", temp_range_bounds[trt_entries]);
    fprintf(fp, "\n");
    for (int i = 0; i < tables; i++) {
        fprintf(fp, "TB%dFC = %d\n", i, frame_counts[i]);
    }
    fprintf(fp, "\n");
    for (int i = 0; i < mode_count; i++) {
        fprintf(fp, "[MODE%d]\n", i);
        if (mode_string) {
            fprintf(fp, "NAME = %s\n", mode_string[i]);
        }
        for (int j = 0; j < trt_entries; j++) {
            fprintf(fp, "T%dTABLE = %d\n", j, wv_modes_temps[i * trt_entries + j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);

    printf("All done!\n");

    free(fn);
    free(temp_range_bounds);
    free(table_offsets);
    free(frame_counts);
    free(wv_modes);
    free(wv_modes_temps);

    free(file_buffer);

    return 0;
}