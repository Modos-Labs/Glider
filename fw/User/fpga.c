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
//#include "bitstream.h"

static int fpga_done = 0;

static void delay_loop(uint32_t t) {
    volatile uint32_t x = t;
    while (x--);
}

uint8_t fpga_write_reg8(uint8_t addr, uint8_t val) {
    uint8_t txbuf[2] = {addr, val};
    uint8_t rxbuf[2];
    gpio_put(FPGA_CS, 0);
    spi_send_recv(FPGA_SPI, txbuf, rxbuf, 2);
    gpio_put(FPGA_CS, 1);
    return rxbuf[1];
}

void fpga_write_reg16(uint8_t addr, uint16_t val) {
    uint8_t txbuf[3] = {addr, val >> 8, val & 0xff};
    gpio_put(FPGA_CS, 0);
    spi_send(FPGA_SPI, txbuf, 3);
    gpio_put(FPGA_CS, 1);
}

void fpga_write_bulk(uint8_t addr, uint8_t *buf, int length) {
    uint8_t txbuf[1] = {addr};
    gpio_put(FPGA_CS, 0);
    spi_send(FPGA_SPI, txbuf, 1);
    spi_send(FPGA_SPI, buf, length);
    gpio_put(FPGA_CS, 1);
}

static void fpga_load_bitstream(const char *fn) {

    TickType_t start = xTaskGetTickCount();

    SPIFFS_clearerr(&spiffs_fs);
    spiffs_file f = SPIFFS_open(&spiffs_fs, fn, SPIFFS_O_RDONLY, 0);
    if (SPIFFS_errno(&spiffs_fs) != 0)
        return;

    spiffs_stat s;
    SPIFFS_fstat(&spiffs_fs, f, &s);
    uint32_t size = s.size;

    const int block_size = 4096;
    uint8_t *buf0 = pvPortMalloc(block_size);
    uint8_t *buf1 = pvPortMalloc(block_size);
    uint8_t *rdbuf = buf0;
    uint8_t *wrbuf = buf1;
    int readbuf = 0;

    gpio_put(FPGA_CS, 0);
    for (int i = 0; i < size / block_size; i++) {
        // Start writing if available
        if (i != 0)
            spi_send_dma(FPGA_SPI, wrbuf, block_size);
        SPIFFS_read(&spiffs_fs, f, rdbuf, block_size);
        // Check DMA finish
        if (i != 0)
            spi_wait_dma_complete(FPGA_SPI);
        // Swap buffer
        readbuf = !readbuf;
        rdbuf = readbuf ? buf1 : buf0;
        wrbuf = readbuf ? buf0 : buf1;
    }

    // Writing last read block
    spi_send_dma(FPGA_SPI, wrbuf, block_size);

    int remaining = size % block_size;
    if (remaining != 0) {
        SPIFFS_read(&spiffs_fs, f, rdbuf, remaining);
    	
        // Check DMA finish
        spi_wait_dma_complete(FPGA_SPI);
        // No need to swap buffer, just send out the last block
        spi_send_dma(FPGA_SPI, rdbuf, block_size);
    }
    gpio_put(FPGA_CS, 1);
    SPIFFS_close(&spiffs_fs, f);

    // Check DMA finish
    spi_wait_dma_complete(FPGA_SPI);

    vPortFree(buf0);
    vPortFree(buf1);

    TickType_t end = xTaskGetTickCount();

    syslog_printf("Bitstream loading took %d ms", (end - start) * (1000 / configTICK_RATE_HZ));
}


static void fpga_wait_done(bool timeout) {
    if (timeout) {
        int i;
        for (i = 0; i < 10; i++) {
            if (gpio_get(FPGA_DONE) == 1)
                break;
            sleep_ms(100);
        }
        if (gpio_get(FPGA_DONE) == 0) {
            syslog_printf("FPGA done does not go high after 1s");
        }
        syslog_printf("FPGA is up after %d ms.\n", i);
    }
    else {
        while (gpio_get(FPGA_DONE) != 1) {
            sleep_ms(100);
        }
        syslog_printf("FPGA is up.\n");
    }
}

void fpga_reset(void) {
    // FPGA Reset
    gpio_put(FPGA_PROG, 0);
    sleep_ms(2);
    gpio_put(FPGA_PROG, 1);
    sleep_ms(10);
}

void fpga_init(const char *fn) {
    // Initialize FPGA pins
    gpio_put(FPGA_CS, 1);

    fpga_reset();

    // Load bitstream
#if 1
    fpga_load_bitstream(fn);
    fpga_wait_done(true);
#else
    //fpga_wait_done(false);
#endif

    // Switch to lower frequency
    board_switch_spi_freq(FPGA_SPI, 6000000);
}

void fpga_suspend(void) {

}

void fpga_resume(void) {

}
