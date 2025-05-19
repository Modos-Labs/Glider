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

#define SPIF_FLASH_SIZE                 0x400000    // 4MB
#define SPIF_SECTOR_SIZE                0x10000     // 64KB
#define SPIF_SUBSECTOR_SIZE             0x1000      // 4KB
#define SPIF_PAGE_SIZE                  0x100       // 256B

#define SPIF_DUMMY_CYCLES_READ          0
#define SPIF_DUMMY_CYCLES_READ_QUAD     6

#define SPIF_BULK_ERASE_TIMEOUT         25000
#define SPIF_SECTOR_ERASE_TIMEOUT       3000
#define SPIF_SUBSECTOR_ERASE_TIMEOUT    800

// Registers
#define SPIF_SR_WIP                     ((uint8_t)0x01)
#define SPIF_SR_WREN                    ((uint8_t)0x02)

#define SPIF_FSR_PRERR                  ((uint8_t)0x02)    /*!< Protection error */
#define SPIF_FSR_PGSUS                  ((uint8_t)0x04)    /*!< Program operation suspended */
#define SPIF_FSR_VPPERR                 ((uint8_t)0x08)    /*!< Invalid voltage during program or erase */
#define SPIF_FSR_PGERR                  ((uint8_t)0x10)    /*!< Program error */
#define SPIF_FSR_ERERR                  ((uint8_t)0x20)    /*!< Erase error */
#define SPIF_FSR_ERSUS                  ((uint8_t)0x40)    /*!< Erase operation suspended */
#define SPIF_FSR_READY                  ((uint8_t)0x80)    /*!< Ready or command in progress */

typedef struct {
    uint8_t manufacturer;
    uint8_t type;
    uint8_t capacity;
    uint8_t id_len;
    uint32_t uid[4];
} spif_id_t;

typedef enum {
    SPIF_READY,
    SPIF_BUSY,
    SPIF_SUSPENDED,
    SPIF_ERROR
} spif_status_t;

extern spiffs spiffs_fs;

int spif_init(void);
int spif_read(uint32_t addr, uint32_t size, uint8_t *dat);
int spif_write(uint32_t addr, uint32_t size, uint8_t *dat);
int spif_erase_block(uint32_t block);
int spif_erase_sector(uint32_t sector);
int spif_erase_chip(void);
spif_status_t spif_get_status(void);
int spif_read_id(spif_id_t *id);
int spif_read_jedec_id(spif_id_t *id);
void spiffs_init(void);
