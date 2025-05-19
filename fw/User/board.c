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

static loop_per_us = 0;

size_t board_usb_get_serial(uint16_t desc_str1[], size_t max_chars) {
    uint8_t uid[16] __attribute__ ((aligned(4)));
    size_t uid_len;
  
    uint32_t* uid32 = (uint32_t*) (uintptr_t) uid;
    *uid32 = board_get_uid();
    uid_len = 4;

    if ( uid_len > max_chars / 2 ) uid_len = max_chars / 2;
  
    for ( size_t i = 0; i < uid_len; i++ ) {
        for ( size_t j = 0; j < 2; j++ ) {
            const char nibble_to_hex[16] = {
                '0', '1', '2', '3', '4', '5', '6', '7',
                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
            };
            uint8_t const nibble = (uid[i] >> (j * 4)) & 0xf;
            desc_str1[i * 2 + (1 - j)] = nibble_to_hex[nibble]; // UTF-16-LE
        }
    }
  
    return 2 * uid_len;
}

void board_late_init(void) {
    uint32_t iter_count = 100;
    uint32_t elapsed_ms;
    do {
        volatile uint32_t x = iter_count;
        uint32_t start = xTaskGetTickCount() / portTICK_PERIOD_MS;
        while (x--);
        uint32_t end = xTaskGetTickCount() / portTICK_PERIOD_MS;
        elapsed_ms = end - start;
        if (elapsed_ms > 10)
            break; // At least 10ms elapsed
        iter_count *= 10;
    } while (1);
    loop_per_us = iter_count / (elapsed_ms * 1000);
    syslog_printf("Delay loop calibrated, iterations per us: %d", loop_per_us);
}

void sleep_us(uint32_t us) {
    volatile uint32_t x = us * loop_per_us;
    while (x--);
}

void board_switch_spi_freq(SPI_HandleTypeDef *spi, uint32_t target) {
    HAL_SPI_DeInit(spi);
    if (target >= 24000000)
        spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    else if (target > 12000000)
        spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    else if (target > 6000000)
        spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    else if (target > 3000000)
        spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    else if (target > 1500000)
        spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    else if (target > 750000)
        spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    else if (target > 375000)
        spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    else
        spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    HAL_SPI_Init(spi);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_15) {
        usbpd_isr();
    }
}

// TODO: Use semaphore?
// TODO: Handle more than 1 SPI instance
static volatile bool tx_complete = false;

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    tx_complete = true;
}

void spi_wait_dma_complete(SPI_HandleTypeDef *spi) {
    while (!tx_complete);
    tx_complete = false;
}
