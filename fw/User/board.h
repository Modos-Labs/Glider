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

#include "pal_i2c.h"

#define LED_GRN         GPIOE, GPIO_PIN_0
#define LED_RED         GPIOE, GPIO_PIN_1

#define BTNCNT          2
#define BTN1            GPIOB, GPIO_PIN_8
#define BTN2            GPIOB, GPIO_PIN_5

#define BTN_PRESSED_LEVEL   1 // Active high

#define FPGA_CS         GPIOB, GPIO_PIN_12
#define FPGA_PROG       GPIOC, GPIO_PIN_10
#define FPGA_DONE       GPIOC, GPIO_PIN_11
#define FPGA_SUSP       GPIOC, GPIO_PIN_12

#define DP_PDN          GPIOD, GPIO_PIN_3
#define DP_HPD          GPIOD, GPIO_PIN_7
#define DEC_RST         GPIOD, GPIO_PIN_6

#define TYPEC_ORI       GPIOA, GPIO_PIN_10
#define TCPC_INT        GPIOA, GPIO_PIN_15

#define EPD_PWREN       GPIOE, GPIO_PIN_11
#define EPD_POSEN       GPIOE, GPIO_PIN_12
#define EPD_THROT       GPIOD, GPIO_PIN_2

#define FL_EN           GPIOE, GPIO_PIN_15

#define VCOM_MEN        GPIOA, GPIO_PIN_2
#define VCOM_EN         GPIOA, GPIO_PIN_3

#define FPGA_SPI        &hspi2

#define PTN3460_I2C     &pi2c1
#define ADV7611_I2C     &pi2c1
#define FUSB302_I2C     &pi2c1
#define INA3221_I2C     &pi2c1

#define TYPEC_MB_ORI_INV    0
#define TYPEC_AUX_ORI_INV   0

#define PTN3460_I2C_ADDR    (0x60)
#define ADV7611_I2C_ADDR    (0x4C)
#define FUSB302_I2C_ADDR    (0x22)
#define INA3221_0_I2C_ADDR  (0x40)
#define INA3221_1_I2C_ADDR  (0x41)

// ADV7611 Sub addresses, make sure they don't conflict with anything else!
#define CEC_I2C_ADDR        (0x3F) // Default 0x40(0x80)
#define INFOFRAME_I2C_ADDR  (0x3E) // Default 0x3E(0x7C)
#define DPLL_I2C_ADDR       (0x26) // Default 0x26(0x4C)
#define KSV_I2C_ADDR        (0x32) // Default 0x32(0x64)
#define EDID_I2C_ADDR       (0x36) // Default 0x36(0x6C)
#define HDMI_I2C_ADDR       (0x34) // Default 0x34(0x68)
#define CP_I2C_ADDR         (0x23) // Default 0x22(0x44)

// For now, these resources are not shared among multiple threads, do not protect them
extern SPI_HandleTypeDef hspi2;
extern ADC_HandleTypeDef hadc1;
extern DAC_HandleTypeDef hdac1;
extern QSPI_HandleTypeDef hqspi;
extern TIM_HandleTypeDef htim1;

// GPIO operations are atomic, no need to protect
#define gpio_put        HAL_GPIO_WritePin
#define gpio_get        HAL_GPIO_ReadPin
#define sleep_ms(x)     vTaskDelay(pdMS_TO_TICKS(x))

#define spi_send(spi, buf, size) \
    HAL_SPI_Transmit(spi, buf, size, HAL_MAX_DELAY)
#define spi_send_dma(spi, buf, size) \
    HAL_SPI_Transmit_DMA(spi, buf, size)
#define spi_send_recv(spi, txbuf, rxbuf, size) \
    HAL_SPI_TransmitReceive(spi, txbuf, rxbuf, size, HAL_MAX_DELAY)

#define board_get_uid   HAL_GetUIDw0

size_t board_usb_get_serial(uint16_t desc_str1[], size_t max_chars);
void board_switch_spi_freq(SPI_HandleTypeDef *spi, uint32_t target);
void board_late_init(void); // Initialization only after RTOS has started
void sleep_us(uint32_t x);
void spi_wait_dma_complete(SPI_HandleTypeDef *spi);
