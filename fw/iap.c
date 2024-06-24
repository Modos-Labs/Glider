//
// Copyright 2024 Wenting Zhang <zephray@outlook.com>
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
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "pico/bootrom.h"
#include "iap.h"

void __no_inline_not_in_flash_func(nuke)() {
    uint flash_size_bytes;
#ifndef PICO_FLASH_SIZE_BYTES
#warning PICO_FLASH_SIZE_BYTES not set, assuming 16M
    flash_size_bytes = 16 * 1024 * 1024;
#else
    flash_size_bytes = PICO_FLASH_SIZE_BYTES;
#endif
    flash_range_erase(0, flash_size_bytes);

    // Pop back up as an MSD drive
    reset_usb_boot(0, 0);
}

void iap_usbboot(void) {
    reset_usb_boot(0, 0);
    __builtin_unreachable();
}

void iap_reset(void) {
    (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C))) = 0x5FA0004; // Reset via NVIC
    __builtin_unreachable();
}

void iap_nuke(void) {
    nuke();
    __builtin_unreachable();
}
