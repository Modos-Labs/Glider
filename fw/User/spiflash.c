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

/* Reset Operations */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

/* Identification Operations */
#define READ_ID_CMD                          0x9E
#define READ_ID_CMD2                         0x9F
#define MULTIPLE_IO_READ_ID_CMD              0xAF
#define READ_SERIAL_FLASH_DISCO_PARAM_CMD    0x5A

/* Read Operations */
#define READ_CMD                             0x03
#define FAST_READ_CMD                        0x0B
#define DUAL_OUT_FAST_READ_CMD               0x3B
#define DUAL_INOUT_FAST_READ_CMD             0xBB
#define QUAD_OUT_FAST_READ_CMD               0x6B
#define QUAD_INOUT_FAST_READ_CMD             0xEB

/* Write Operations */
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04

/* Register Operations */
#define READ_STATUS_REG_CMD                  0x05
#define WRITE_STATUS_REG_CMD                 0x01

#define READ_LOCK_REG_CMD                    0xE8
#define WRITE_LOCK_REG_CMD                   0xE5

#define READ_FLAG_STATUS_REG_CMD             0x70
#define CLEAR_FLAG_STATUS_REG_CMD            0x50

#define READ_NONVOL_CFG_REG_CMD              0xB5
#define WRITE_NONVOL_CFG_REG_CMD             0xB1

#define READ_VOL_CFG_REG_CMD                 0x85
#define WRITE_VOL_CFG_REG_CMD                0x81

#define READ_ENHANCED_VOL_CFG_REG_CMD        0x65
#define WRITE_ENHANCED_VOL_CFG_REG_CMD       0x61

/* Program Operations */
#define PAGE_PROG_CMD                        0x02
#define DUAL_IN_FAST_PROG_CMD                0xA2
#define EXT_DUAL_IN_FAST_PROG_CMD            0xD2
#define QUAD_IN_FAST_PROG_CMD                0x32
#define EXT_QUAD_IN_FAST_PROG_CMD            0x12

/* Erase Operations */
#define SUBSECTOR_ERASE_CMD                  0x20
#define SECTOR_ERASE_CMD                     0xD8
#define BULK_ERASE_CMD                       0xC7

#define PROG_ERASE_RESUME_CMD                0x7A
#define PROG_ERASE_SUSPEND_CMD               0x75

/* One-Time Programmable Operations */
#define READ_OTP_ARRAY_CMD                   0x4B
#define PROG_OTP_ARRAY_CMD                   0x42

static int spif_auto_polling_mem_ready(uint32_t timeout) {
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;
  
    /* Configure automatic polling mode to wait for memory ready */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_STATUS_REG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
  
    s_config.Match           = 0;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;
    s_config.Mask            = SPIF_SR_WIP;
    s_config.StatusBytesSize = 2;
  
    if (HAL_QSPI_AutoPolling(&hqspi, &s_command, &s_config, timeout) != HAL_OK)
        return -1;
  
    return 0;
}

static int spif_reset_memory(void) {
    QSPI_CommandTypeDef     s_command;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = RESET_ENABLE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    s_command.Instruction       = RESET_MEMORY_CMD;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    return spif_auto_polling_mem_ready(HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static int spif_write_enable(void) {
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;

    /* Enable write operations */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = WRITE_ENABLE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    /* Configure automatic polling mode to wait for write enabling */
    s_config.Match           = SPIF_SR_WREN;
    s_config.Mask            = SPIF_SR_WREN;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    s_command.Instruction    = READ_STATUS_REG_CMD;
    s_command.DataMode       = QSPI_DATA_1_LINE;

    if (HAL_QSPI_AutoPolling(&hqspi, &s_command, &s_config, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;
}

static int spif_dummy_cycles_cfg(void) {
    QSPI_CommandTypeDef s_command;
    uint8_t reg = 0;

    /* Initialize the read volatile configuration register command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_VOL_CFG_REG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 1;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    /* Reception of the data */
    if (HAL_QSPI_Receive(&hqspi, (uint8_t *)(&reg), HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    /* Enable write operations */
    if (spif_write_enable() != 0)
        return -1;

    /* Update volatile configuration register (with new dummy cycles) */
    s_command.Instruction = WRITE_VOL_CFG_REG_CMD;
    MODIFY_REG(reg, 0xF0F0, (SPIF_DUMMY_CYCLES_READ_QUAD << 4));

    /* Configure the write volatile configuration register command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    /* Transmission of the data */
    if (HAL_QSPI_Transmit(&hqspi, (uint8_t *)(&reg), HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    return 0;
}

int spif_init(void) {
    int result;
    result = spif_reset_memory();
    //result += spif_dummy_cycles_cfg();
    if (result != 0) {
    	syslog_printf("SPI flash initialization failed");
    }
    return result;
}

// TODO: Should have been a semaphore
static volatile bool tx_complete = false;
static volatile bool rx_complete = false;

void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi) {
    tx_complete = true;
}

void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi) {
    rx_complete = true;
}

int spif_read(uint32_t addr, uint32_t size, uint8_t *dat) {
    static QSPI_CommandTypeDef s_command = {
        .InstructionMode   = QSPI_INSTRUCTION_1_LINE,
        .Instruction       = QUAD_INOUT_FAST_READ_CMD,
        .AddressMode       = QSPI_ADDRESS_4_LINES,
        .AddressSize       = QSPI_ADDRESS_24_BITS,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_4_LINES,
        .DummyCycles       = SPIF_DUMMY_CYCLES_READ_QUAD,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD,
    };

    /* Initialize the read command */
    s_command.Address           = addr;
    s_command.NbData            = size;

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    rx_complete = false;

    /* Reception of the data */
    if (HAL_QSPI_Receive_DMA(&hqspi, dat) != HAL_OK)
        return -1;

    while (!rx_complete);

    return 0;
}

int spif_write(uint32_t addr, uint32_t size, uint8_t *dat) {
    QSPI_CommandTypeDef s_command;
    uint32_t end_addr, current_size, current_addr;

    /* Calculation of the size between the write address and the end of the page */
    current_size = SPIF_PAGE_SIZE - (addr % SPIF_PAGE_SIZE);

    /* Check if the size of the data is less than the remaining place in the page */
    if (current_size > size) {
        current_size = size;
    }

    /* Initialize the adress variables */
    current_addr = addr;
    end_addr = addr + size;

    /* Initialize the program command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = QUAD_IN_FAST_PROG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Perform the write page by page */
    do {
        s_command.Address = current_addr;
        s_command.NbData  = current_size;

        if (spif_write_enable() != 0)
            return -1;

        if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
            return -1;

        tx_complete = false;

        if (HAL_QSPI_Transmit_DMA(&hqspi, dat) != HAL_OK)
            return -1;

        while (!tx_complete);

        if (spif_auto_polling_mem_ready(HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != 0)
            return -1;

        current_addr += current_size;
        dat += current_size;
        current_size = ((current_addr + SPIF_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : SPIF_PAGE_SIZE;
    } while (current_addr < end_addr);

    return 0;
}

int spif_erase_block(uint32_t block) {
    QSPI_CommandTypeDef s_command;

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = SUBSECTOR_ERASE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = block;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (spif_write_enable() != 0)
        return -1;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    if (spif_auto_polling_mem_ready(SPIF_SUBSECTOR_ERASE_TIMEOUT) != 0)
        return -1;
}

int spif_erase_sector(uint32_t sector) {
    QSPI_CommandTypeDef s_command;

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = SECTOR_ERASE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = (sector * SPIF_SECTOR_SIZE);
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (spif_write_enable() != 0)
        return -1;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    if (spif_auto_polling_mem_ready(SPIF_SECTOR_ERASE_TIMEOUT) != 0)
        return -1;
}

int spif_erase_chip(void) {
    QSPI_CommandTypeDef s_command;

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = BULK_ERASE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (spif_write_enable() != 0)
        return -1;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    if (spif_auto_polling_mem_ready(SPIF_BULK_ERASE_TIMEOUT) != 0)
        return -1;
}

uint8_t spif_get_status(void) {
    QSPI_CommandTypeDef s_command;
    uint8_t reg;

    /* Initialize the read flag status register command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_FLAG_STATUS_REG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 1;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    if (HAL_QSPI_Receive(&hqspi, &reg, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    if (reg & (SPIF_FSR_PRERR | SPIF_FSR_VPPERR | SPIF_FSR_PGERR | SPIF_FSR_ERERR))
        return SPIF_ERROR;
    else if (reg & (SPIF_FSR_PGSUS | SPIF_FSR_ERSUS))
        return SPIF_SUSPENDED;
    else if (reg & SPIF_FSR_READY)
        return SPIF_READY;
    else
        return SPIF_BUSY;
}

int spif_read_id(spif_id_t *id) {
    uint8_t *rxbuf = (uint8_t *)id;
    QSPI_CommandTypeDef s_command;

    memset(id, 0, sizeof(*id));

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_ID_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 20;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    if (HAL_QSPI_Receive(&hqspi, rxbuf, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    return 0;
}

int spif_read_jedec_id(spif_id_t *id) {
    uint8_t *rxbuf = (uint8_t *)id;
    QSPI_CommandTypeDef s_command;

    memset(id, 0, sizeof(*id));

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_ID_CMD2;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 3;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    if (HAL_QSPI_Receive(&hqspi, rxbuf, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    return 0;
}

// spiffs abstraction layer
static spiffs_config spiffs_cfg;
spiffs spiffs_fs;
SemaphoreHandle_t spiffs_lock;
static uint8_t fs_work_buf[256 * 2];
static uint8_t fs_fds[32 * 4];
static uint8_t fs_cache_buf[(256 + 32) * 4];

static int32_t _spiffs_erase(uint32_t addr, uint32_t len) {
    uint32_t i = 0;
    uint32_t erase_count = (len + 4096 - 1) / 4096;
    int res = 0;
    for (i = 0; i < erase_count; i++) {
        res += spif_erase_block(addr + i * 4096);
    }
    return 0;
}

static int32_t _spiffs_read(uint32_t addr, uint32_t size, uint8_t *dst) {
    return spif_read(addr, size, dst);
}

static int32_t _spiffs_write(uint32_t addr, uint32_t size, uint8_t *dst) {
    return spif_write(addr, size, dst);
}

void spiffs_init(void) {
	spiffs_lock = xSemaphoreCreateMutex();
	spiffs_cfg.hal_erase_f = _spiffs_erase;
	spiffs_cfg.hal_read_f = _spiffs_read;
	spiffs_cfg.hal_write_f = _spiffs_write;
	int res = SPIFFS_mount(&spiffs_fs, &spiffs_cfg, fs_work_buf, fs_fds,
			sizeof(fs_fds), fs_cache_buf, sizeof(fs_cache_buf), NULL);
	if ((res != SPIFFS_OK) && (SPIFFS_errno(&spiffs_fs) == SPIFFS_ERR_NOT_A_FS)) {
		syslog_printf("Formatting SPIFFS...");
		if (SPIFFS_format(&spiffs_fs) != SPIFFS_OK) {
			syslog_printf("SPIFFS format failed: %d\n", SPIFFS_errno(&spiffs_fs));
		}
		res = SPIFFS_mount(&spiffs_fs, &spiffs_cfg, fs_work_buf, fs_fds,
				sizeof(fs_fds), fs_cache_buf, sizeof(fs_cache_buf), NULL);
	}
	if (res != SPIFFS_OK) {
		syslog_printf("SPIFFS mount failed: %d\n", SPIFFS_errno(&spiffs_fs));
	}
	else {
		syslog_printf("SPIFFS mounted\n");
	}
}
