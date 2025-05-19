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
#include "pal_i2c.h"

// Ugly
#define I2C1_SCL        GPIOB, GPIO_PIN_6
#define I2C1_SDA        GPIOB, GPIO_PIN_7

struct pal_i2c_t {
    SemaphoreHandle_t lock;
    I2C_TypeDef *port;
};

struct pal_i2c_t pi2c1;

extern I2C_HandleTypeDef hi2c1;

void pal_i2c_init(void) {
    pi2c1.port = I2C1;
    pi2c1.lock = xSemaphoreCreateMutex();
}

bool pal_i2c_ll_start(pal_i2c_t *i2c, uint32_t request, uint8_t slave_addr, uint8_t transfer_size) {
    I2C_TypeDef* port = i2c->port;
    int timeout = 500;

    LL_I2C_HandleTransfer(port, slave_addr, LL_I2C_ADDRSLAVE_7BIT,
        transfer_size, LL_I2C_MODE_SOFTEND, request);

    while (!LL_I2C_IsActiveFlag_TXIS(port) && !LL_I2C_IsActiveFlag_RXNE(port)) {
        if (LL_I2C_IsActiveFlag_NACK(port)) {
        	syslog_printf("Addr NACK");
            LL_I2C_ClearFlag_NACK(port);
            return false;
        }
        if (timeout-- == 0) {
        	syslog_printf("Addr timeout");
        	return false;
        }
        sleep_us(1);
    }

    return true;
}


bool pal_i2c_ll_send(pal_i2c_t *i2c, uint8_t val) {
    I2C_TypeDef* port = i2c->port;
    int timeout = 500;

    LL_I2C_TransmitData8(port, val);
    while (!LL_I2C_IsActiveFlag_TXIS(port) && !LL_I2C_IsActiveFlag_TC(port)) {
        /* Break if ACK failed */
        if (LL_I2C_IsActiveFlag_NACK(port)) {
            LL_I2C_ClearFlag_NACK(port);
            syslog_printf("Data NACK");
            return false;
        }
        if (timeout-- == 0) {
        	syslog_printf("Data timeout");
        	return false;
        }
        sleep_us(1);
    }

    return true;
}


bool pal_i2c_ll_recv(pal_i2c_t *i2c, uint8_t *val) {
    I2C_TypeDef* port = i2c->port;
    int timeout = 500;

    if (!val)
        return false;

    while (!LL_I2C_IsActiveFlag_RXNE(port) && !LL_I2C_IsActiveFlag_TC(port)) {
        if (timeout-- == 0) {
        	return false;
        }
        sleep_us(1);
    }

    *val = LL_I2C_ReceiveData8(port);

    return true;
}


void pal_i2c_ll_stop(pal_i2c_t *i2c) {
    I2C_TypeDef* port = i2c->port;
    /* Send STOP bit */
    LL_I2C_GenerateStopCondition(port);
    while(LL_I2C_IsActiveFlag_BUSY(port));
}

void pal_i2c_ll_lock(pal_i2c_t *i2c) {
    xSemaphoreTake(i2c->lock, portMAX_DELAY);
}

void pal_i2c_ll_unlock(pal_i2c_t *i2c) {
    xSemaphoreGive(i2c->lock);
}

int pal_i2c_write_byte(pal_i2c_t *i2c, uint8_t addr, uint8_t val) {
    int result = -1;
    xSemaphoreTake(i2c->lock, portMAX_DELAY);
    if (!pal_i2c_ll_start(i2c, REQ_WRITE, addr << 1, 1))
        goto i2c_stop_point;
    if (!pal_i2c_ll_send(i2c, val))
        goto i2c_stop_point;
    result = 0;
i2c_stop_point:
    pal_i2c_ll_stop(i2c);
    xSemaphoreGive(i2c->lock);
    return result;
}

int pal_i2c_write_reg(pal_i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t val) {
    int result = -1;
    xSemaphoreTake(i2c->lock, portMAX_DELAY);
    if (!pal_i2c_ll_start(i2c, REQ_WRITE, addr << 1, 2))
        goto i2c_stop_point;
    if (!pal_i2c_ll_send(i2c, reg))
        goto i2c_stop_point;
    if (!pal_i2c_ll_send(i2c, val))
        goto i2c_stop_point;
    result = 0;
i2c_stop_point:
    pal_i2c_ll_stop(i2c);
    xSemaphoreGive(i2c->lock);
    return result;
}

int pal_i2c_write_longreg(pal_i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t *payload, size_t len) {
    int result = -1;
    xSemaphoreTake(i2c->lock, portMAX_DELAY);
    if (!pal_i2c_ll_start(i2c, REQ_WRITE, addr << 1, 1 + len))
        goto i2c_stop_point;
    if (!pal_i2c_ll_send(i2c, reg))
        goto i2c_stop_point;
    for (int i = 0; i < len; i++) {
        if (!pal_i2c_ll_send(i2c, payload[i]))
            goto i2c_stop_point;
    }
    result = 0;
i2c_stop_point:
    pal_i2c_ll_stop(i2c);
    xSemaphoreGive(i2c->lock);
    return result;
}

int pal_i2c_write_payload(pal_i2c_t *i2c, uint8_t addr, uint8_t *payload, size_t len) {
    int result = -1;
    xSemaphoreTake(i2c->lock, portMAX_DELAY);
    if (!pal_i2c_ll_start(i2c, REQ_WRITE, addr << 1, len))
        goto i2c_stop_point;
    for (int i = 0; i < len; i++) {
        if (!pal_i2c_ll_send(i2c, payload[i]))
            goto i2c_stop_point;
    }
    result = 0;
i2c_stop_point:
    pal_i2c_ll_stop(i2c);
    xSemaphoreGive(i2c->lock);
    return result;
}

int pal_i2c_read_reg(pal_i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t *val) {
    int result = -1;
    xSemaphoreTake(i2c->lock, portMAX_DELAY);
    if (!pal_i2c_ll_start(i2c, REQ_WRITE, addr << 1, 1))
        goto i2c_stop_point;
    if (!pal_i2c_ll_send(i2c, reg))
        goto i2c_stop_point;
    if (!pal_i2c_ll_start(i2c, REQ_RESTART_READ, addr << 1, 1))
        goto i2c_stop_point;
    if (!pal_i2c_ll_recv(i2c, val))
        goto i2c_stop_point;
    result = 0;
i2c_stop_point:
    pal_i2c_ll_stop(i2c);
    xSemaphoreGive(i2c->lock);
    return result;
}

int pal_i2c_read_payload(pal_i2c_t *i2c, uint8_t addr, uint8_t *tx_payload, size_t tx_len, uint8_t *rx_payload, size_t rx_len) {
    int result = -1;
    xSemaphoreTake(i2c->lock, portMAX_DELAY);
    if (!pal_i2c_ll_start(i2c, REQ_WRITE, addr << 1, tx_len))
        goto i2c_stop_point;
    for (int i = 0; i < tx_len; i++) {
        if (!pal_i2c_ll_send(i2c, tx_payload[i]))
            goto i2c_stop_point;
    }
    if (!pal_i2c_ll_start(i2c, REQ_RESTART_READ, addr << 1, rx_len))
        goto i2c_stop_point;
    for (int i = 0; i < rx_len; i++) {
        if (!pal_i2c_ll_recv(i2c, &rx_payload[i]))
            goto i2c_stop_point;
    }
    result = 0;
i2c_stop_point:
    pal_i2c_ll_stop(i2c);
    xSemaphoreGive(i2c->lock);
    return result;
}

int pal_i2c_read_byte(pal_i2c_t *i2c, uint8_t addr, uint8_t *val) {
    int result = -1;
    xSemaphoreTake(i2c->lock, portMAX_DELAY);
    if (!pal_i2c_ll_start(i2c, REQ_READ, addr << 1, 1))
        goto i2c_stop_point;
    if (!pal_i2c_ll_recv(i2c, val))
        goto i2c_stop_point;
    result = 0;
i2c_stop_point:
    pal_i2c_ll_stop(i2c);
    xSemaphoreGive(i2c->lock);
    return result;
}

bool pal_i2c_ping(pal_i2c_t *i2c, uint8_t addr) {
    bool result = true;
    I2C_TypeDef* port = i2c->port;
    xSemaphoreTake(i2c->lock, portMAX_DELAY);

    // Switch to GPIO emulated I2C
    HAL_GPIO_WritePin(I2C1_SDA, 1);
    HAL_GPIO_WritePin(I2C1_SCL, 1);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Start Condition
    sleep_us(10);
    HAL_GPIO_WritePin(I2C1_SDA, 0);
    sleep_us(5);
    HAL_GPIO_WritePin(I2C1_SCL, 0);
    sleep_us(10);

    // Send 7-bit address
    uint8_t dat = addr << 1;
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(I2C1_SDA, !!(dat & 0x80));
        dat <<= 1;
        sleep_us(5);
        HAL_GPIO_WritePin(I2C1_SCL, 1);
        sleep_us(10);
        HAL_GPIO_WritePin(I2C1_SCL, 0);
        sleep_us(5);
    }
    HAL_GPIO_WritePin(I2C1_SDA, 1);
    sleep_us(5);
    HAL_GPIO_WritePin(I2C1_SCL, 1);
    sleep_us(10);
    int ack = HAL_GPIO_ReadPin(I2C1_SDA); // 0 - ack, 1 - nack
    HAL_GPIO_WritePin(I2C1_SCL, 0);

    // Stop condition
    sleep_us(10);
    HAL_GPIO_WritePin(I2C1_SDA, 0);
    sleep_us(5);
    HAL_GPIO_WritePin(I2C1_SCL, 1);
    sleep_us(5);
    HAL_GPIO_WritePin(I2C1_SDA, 1);

    // Back to hardware I2C
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    xSemaphoreGive(i2c->lock);
    return !ack;
}
