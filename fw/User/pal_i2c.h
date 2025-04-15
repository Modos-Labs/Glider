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

// Opaque I2C handler
typedef struct pal_i2c_t pal_i2c_t;

extern pal_i2c_t pi2c1;

void pal_i2c_init(void);
int pal_i2c_write_byte(pal_i2c_t *i2c, uint8_t addr, uint8_t val);
int pal_i2c_write_reg(pal_i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t val);
int pal_i2c_write_longreg(pal_i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t *payload, size_t len);
int pal_i2c_write_payload(pal_i2c_t *i2c, uint8_t addr, uint8_t *payload, size_t len);
int pal_i2c_read_reg(pal_i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t *val);
int pal_i2c_read_payload(pal_i2c_t *i2c, uint8_t addr, uint8_t *tx_payload, size_t tx_len, uint8_t *rx_payload, size_t rx_len);
int pal_i2c_read_byte(pal_i2c_t *i2c, uint8_t addr, uint8_t *val);
bool pal_i2c_ping(pal_i2c_t *i2c, uint8_t addr);

// LL functions:
#define REQ_NONE            LL_I2C_GENERATE_NOSTARTSTOP
#define REQ_WRITE           LL_I2C_GENERATE_START_WRITE
#define REQ_READ            LL_I2C_GENERATE_START_READ
#define REQ_RESTART_READ    LL_I2C_GENERATE_RESTART_7BIT_READ
#define REQ_RELOAD			LL_I2C_MODE_RELOAD

bool pal_i2c_ll_start(pal_i2c_t *i2c, uint32_t request, uint8_t slave_addr, uint8_t transfer_size);
bool pal_i2c_ll_send(pal_i2c_t *i2c, uint8_t val);
bool pal_i2c_ll_recv(pal_i2c_t *i2c, uint8_t *val);
void pal_i2c_ll_stop(pal_i2c_t *i2c);
void pal_i2c_ll_lock(pal_i2c_t *i2c);
void pal_i2c_ll_unlock(pal_i2c_t *i2c);
