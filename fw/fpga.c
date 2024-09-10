//
// Copyright 2022 Wenting Zhang <zephray@outlook.com>
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
#include "pico/stdlib.h"
#include <stdio.h>
#include "utils.h"
#include "config.h"
#include "fpga.h"
#include "bitstream.h"

static int fpga_done = 0;

static void gpio_init_out(uint32_t pin, bool val) {
    gpio_init(pin);
    gpio_put(pin, val);
    gpio_set_dir(pin, GPIO_OUT);
}

static void gpio_init_ipu(uint32_t pin) {
    gpio_init(pin);
    gpio_pull_up(pin);
}

static void delay_loop(uint32_t t) {
    volatile uint32_t x = t;
    while (x--);
}

static void fpga_send_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        gpio_put(FPGA_MOSI, byte & 0x80);
        gpio_put(FPGA_SCLK, 1);
        byte <<= 1;
        gpio_put(FPGA_SCLK, 0);
    }
}

static uint8_t fpga_send_byte_slow(uint8_t byte) {
    uint8_t rxbyte;
    for (int i = 0; i < 8; i++) {
        gpio_put(FPGA_MOSI, byte & 0x80);
        delay_loop(20);
        rxbyte |= gpio_get(FPGA_MISO);
        gpio_put(FPGA_SCLK, 1);
        delay_loop(20);
        byte <<= 1;
        rxbyte <<= 1;
        gpio_put(FPGA_SCLK, 0);
    }
    delay_loop(20);
    rxbyte |= gpio_get(FPGA_MISO);
    gpio_put(FPGA_SCLK, 1);
    return rxbyte;
}

uint8_t fpga_write_reg8(uint8_t addr, uint8_t val) {
    uint8_t oldval;
    gpio_put(FPGA_CS, 0);
    fpga_send_byte_slow(addr);
    oldval = fpga_send_byte_slow(val);
    gpio_put(FPGA_CS, 1);
    return oldval;
}

void fpga_write_reg16(uint8_t addr, uint16_t val) {
    gpio_put(FPGA_CS, 0);
    fpga_send_byte_slow(addr);
    fpga_send_byte_slow(val >> 8);
    fpga_send_byte_slow(val & 0xff);
    gpio_put(FPGA_CS, 1);
}

void fpga_write_bulk(uint8_t addr, uint8_t *buf, int length) {
    gpio_put(FPGA_CS, 0);
    fpga_send_byte_slow(addr);
    for (int i = 0; i < length; i++) {
        fpga_send_byte_slow(buf[i]);
    }
    gpio_put(FPGA_CS, 1);
}

static void fpga_load_bitstream(uint8_t *stream, int size) {
    gpio_put(FPGA_CS, 0);
    for (int i = 0; i < size; i++) {
        fpga_send_byte(stream[i]);
    }
    gpio_put(FPGA_CS, 1);
    printf("FPGA bitstream load done.\n");
}

static void fpga_wait_done(bool timeout) {
    if (timeout) {
        int i;
        for (i = 0; i < 1000; i++) {
            if (gpio_get(FPGA_DONE) == 1)
                break;
            sleep_ms(1);
        }
        if (gpio_get(FPGA_DONE) == 0) {
            fatal("FPGA done does not go high after 1s");
        }
        printf("FPGA is up after %d ms.\n", i);
    }
    else {
        while (gpio_get(FPGA_DONE) != 1);
        printf("FPGA is up.\n");
    }
}

void fpga_init(void) {
    // Initialize FPGA pins
    gpio_init_out(FPGA_CS, 1);
    gpio_init_out(FPGA_MOSI, 1);
    gpio_init_ipu(FPGA_MISO);
    gpio_init_out(FPGA_SCLK, 0);
    gpio_init_out(FPGA_PROG, 1);
    gpio_init_ipu(FPGA_DONE);
    gpio_init_out(FPGA_SUSP, 0);
    
    gpio_put(FPGA_CS, 1);

    // FPGA Reset
    gpio_put(FPGA_PROG, 0);
    sleep_ms(100);
    gpio_put(FPGA_PROG, 1);
    sleep_ms(100);

    // Load bitstream
#if 1
    fpga_load_bitstream(fpga_bitstream, fpga_bitstream_length);
    fpga_wait_done(true);
#else
    fpga_wait_done(false);
#endif


}

void fpga_suspend(void) {

}

void fpga_resume(void) {

}
