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

#include "shell.h"
#include "syslog.h"
#include "usbapp.h"
#include "crc16.h"
#include "ptn3460.h"
#include "config.h"
#include "edid.h"
#include "fpga.h"
#include "adv7611.h"
#include "usb_pd.h"
#include "usbpd.h"
#include "spiffs.h"
#include "spiffs_config.h"
#include "spiffs_nucleus.h"
#include "spiflash.h"
#include "power.h"
#include "caster.h"
#include "button.h"
#include "ui.h"
#include "fonts.h"

//#define UNUSED(expr) do { (void)(expr); } while (0)

#define HOUSEKEEPING_TASK_PRIORITY      (tskIDLE_PRIORITY + 1)
#define STARTUP_TASK_LOW_PRIORITY       (tskIDLE_PRIORITY + 1)
#define UI_TASK_PRIORITY                (tskIDLE_PRIORITY + 3)
#define USB_DEVICE_TASK_PRIORITY        (tskIDLE_PRIORITY + 4)
#define USB_PD_TASK_PRIORITY            (tskIDLE_PRIORITY + 4)
#define STARTUP_TASK_HIGH_PRIORITY      (tskIDLE_PRIORITY + 5)
#define KEY_SCAN_TASK_PRIORITY          (tskIDLE_PRIORITY + 5)
#define POWER_MON_TASK_PRIORITY         (tskIDLE_PRIORITY + 5)

#define STARTUP_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE + 1024)
#define USB_DEVICE_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE + 128)
#define USB_PD_TASK_STACK_SIZE          (configMINIMAL_STACK_SIZE + 128)
#define HOUSEKEEPING_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE)
#define UI_TASK_STACK_SIZE              (configMINIMAL_STACK_SIZE + 256)
#define KEY_SCAN_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE)
#define POWER_MON_TASK_STACK_SIZE       (configMINIMAL_STACK_SIZE + 256)
