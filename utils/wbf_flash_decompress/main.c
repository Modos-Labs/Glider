// Eink waveform flash decompressor
// Copyright 2024 Wenting Zhang
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>
#include <ctype.h>

#define MAX_DECOMP_SIZE (0x100000)

static uint32_t read_uint16_be(uint8_t *ptr) {
    uint32_t b0 = (uint32_t)(*ptr++) << 8;
    uint32_t b1 = (uint32_t)(*ptr++);
    return b0 | b1;
}

static uint32_t read_uint16_le(uint8_t *ptr) {
    uint32_t b0 = (uint32_t)(*ptr++);
    uint32_t b1 = (uint32_t)(*ptr++) << 8;
    return b0 | b1;
}

static uint32_t read_uint32_be(uint8_t *ptr) {
    uint32_t b0 = (uint32_t)(*ptr++) << 24;
    uint32_t b1 = (uint32_t)(*ptr++) << 16;
    uint32_t b2 = (uint32_t)(*ptr++) << 8;
    uint32_t b3 = (uint32_t)(*ptr++);
    return b0 | b1 | b2 | b3;
}

int main(int argc, char **argv) {
    fprintf(stderr, "Eink waveform flash decompressor\n");

    if (argc != 3) {
        fprintf(stderr, "Usage: wbf_flash_decompress input_file output_file\n");
        fprintf(stderr, "input_file: Compressed flash ROM (like a flash dump)\n");
        fprintf(stderr, "output_file: WBF file name\n");
        return 1;
    }

    char *bin = argv[1];
    char *wbf = argv[2];

    FILE * fp;
    size_t file_size;
    char * file_buffer;
    size_t result;

    fp = fopen(bin, "rb");
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

    printf("File size: %zu bytes\n", file_size);

    uint8_t *ptr = (uint8_t *)file_buffer;
    uint32_t compressed_len = read_uint32_be(ptr);
    ptr += 4;
    uint32_t header_version = read_uint32_be(ptr);

    printf("Compressed length: %d bytes\n", compressed_len);

    if ((compressed_len + 16) > file_size) {
        printf("Error: file size smaller than expected.\n");
        return -1;
    }
    else if ((compressed_len + 16) < file_size) {
        printf("Warning: file size larger than expected.\n");
    }

    printf("Header version: %d\n", header_version);

    if (header_version != 1) {
        printf("Unsupported file version\n");
        return -1;
    }

    // Start decompress
    ptr = (uint8_t *)file_buffer + 16; // skip header
    static uint8_t decomp_buffer[MAX_DECOMP_SIZE];
    uint32_t wrptr = 0;
    uint8_t *ptr_end = ptr + compressed_len;
    while (ptr < ptr_end) { 
        uint32_t wrptr_snapshot = wrptr;
        uint32_t offset = (uint32_t)read_uint16_le(ptr);
        ptr += 2;
        uint8_t len = *ptr++;
        uint8_t byte = *ptr++;
        printf("Ptr %d, Offset %d, Len %d, Byte 0x%02x\n", wrptr_snapshot, offset, len, byte);
        if (len != 0) {
            printf("Src: %d\n", wrptr_snapshot - offset);
        }
        for (int i = 0; i < len; i++) {
            // len could be zero, in this case the loop doesn't execute
            uint8_t rd = decomp_buffer[wrptr_snapshot - offset + i];
            decomp_buffer[wrptr++] = rd;
        }
        decomp_buffer[wrptr++] = byte;
    }

    printf("Decompressed size: %d bytes\n", wrptr);

    fp = fopen(wbf, "wb");
    fwrite(decomp_buffer, wrptr, 1, fp);
    fclose(fp);
    printf("Done\n");

    return 0;
}