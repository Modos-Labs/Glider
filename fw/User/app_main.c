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

// Memory for FreeRTOS
//__attribute__((section(".mysection"))) uint8_t ucHeap[configTOTAL_HEAP_SIZE];
uint8_t ucHeap[configTOTAL_HEAP_SIZE];

// Contexts
static shell_context_t shell;

// Task handles
TaskHandle_t housekeeping_task_handle;
TaskHandle_t startup_task_handle;
TaskHandle_t idle_task_handle;
TaskHandle_t usb_device_task_handle;
TaskHandle_t usb_pd_task_handle;
TaskHandle_t ui_task_handle;
TaskHandle_t key_scan_task_handle;
TaskHandle_t power_mon_task_handle;

static portTASK_FUNCTION(housekeeping_task, pvParameters) {
    int led = 0;
    TickType_t last_flash_time, flash_rate;

    flash_rate = pdMS_TO_TICKS(500);
    last_flash_time = xTaskGetTickCount();

    while (1) {
        gpio_put(LED_GRN, led);
        led = !led;
        vTaskDelayUntil(&last_flash_time, flash_rate);  
    }
}

static portTASK_FUNCTION(startup_task, pvParameters) {
    // Power up sequence continues here
    syslog_printf("System starting");
    syslog_printf("Serial number: %08x", board_get_uid());

    board_late_init();
    pal_i2c_init();
    spif_init();
    spif_id_t id;
    spif_read_jedec_id(&id);
    syslog_printf("SPI Flash Mfg ID: %02x\n", id.manufacturer);
    syslog_printf("SPI Flash Type: %02x\n", id.type);
    syslog_printf("SPI Flash Capacity: %02x\n", id.capacity);
    spiffs_init();
//    spif_erase_sector(0);
//    uint8_t buf[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
//    spif_write(0, 8, buf);
//    memset(buf, 0, 8);
//    spif_read(0, 8, buf);
//    syslog_printf("RD: %02x %02x %02x %02x %02x %02x %02x %02x", buf[0], buf[1],
//        buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    config_init();
    config_load();
    edid_init();
    adv7611_early_init(); // Must be before PTN3460 to release RST and I2C bus
    ptn3460_early_init(); // Let PTN3460 starts internal bootup process
    fpga_init();
    adv7611_init();
    ptn3460_init();
    caster_init(); // must be after adv7611 as it's the clock source for CSR interface
    power_set_vcom(config.vcom); // Move out from here
    power_set_vgh(config.vgh);
    ui_init();

    idle_task_handle = xTaskGetIdleTaskHandle();

    xTaskCreate(housekeeping_task, "HousekeepingTask", HOUSEKEEPING_TASK_STACK_SIZE,
        NULL, HOUSEKEEPING_TASK_PRIORITY, &housekeeping_task_handle);
    xTaskCreate(usb_device_task, "USBDeviceTask", USB_DEVICE_TASK_STACK_SIZE,
        NULL, USB_DEVICE_TASK_PRIORITY, &usb_device_task_handle);
    xTaskCreate(usb_pd_task, "USBPDTask", USB_PD_TASK_STACK_SIZE,
        NULL, USB_PD_TASK_PRIORITY, &usb_pd_task_handle);
    xTaskCreate(ui_task, "UITask", UI_TASK_STACK_SIZE,
        NULL, UI_TASK_PRIORITY, &ui_task_handle);
    xTaskCreate(key_scan_task, "KeyScanTask", KEY_SCAN_TASK_STACK_SIZE,
        NULL, KEY_SCAN_TASK_PRIORITY, &key_scan_task_handle);
    xTaskCreate(power_monitor_task, "PowerMonitorTask", POWER_MON_TASK_STACK_SIZE,
        NULL, POWER_MON_TASK_PRIORITY, &power_mon_task_handle);

    vTaskPrioritySet(NULL, STARTUP_TASK_LOW_PRIORITY);
    
    shell_init(&shell, usbapp_term_out, usbapp_term_in, SHELL_MODE_BLOCKING, NULL);

    while (1) {
        shell_start(&shell);
    }
}

void app_init(void) {
    syslog_init();

    xTaskCreate(startup_task, "StartupTask", STARTUP_TASK_STACK_SIZE,
        NULL, STARTUP_TASK_HIGH_PRIORITY, &startup_task_handle);

    // Up to CubeMX to generate RTOS start
}
