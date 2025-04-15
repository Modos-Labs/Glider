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

static SemaphoreHandle_t isr_sem = NULL;

void usbpd_isr(void) {
    BaseType_t context_switch = pdFALSE;

    if (isr_sem) {
        xSemaphoreGiveFromISR(isr_sem, &context_switch);
        portYIELD_FROM_ISR(context_switch);
    }
}

portTASK_FUNCTION(usb_pd_task, pvParameters) {
    isr_sem = xSemaphoreCreateCounting(1, 0);
    extern int dp_enabled;
    static bool hpd_sent = false;

    int result = tcpm_init(0);
    if (result)
    	syslog_printf("Failed to initialize TCPC\n");

    int cc1, cc2;
    tcpc_config[0].drv->get_cc(0, &cc1, &cc2);
    syslog_printf("CC status %d %d\n", cc1, cc2);
    pd_init(0);
    sleep_ms(50);
    while (1) {
        BaseType_t rtos_result = xSemaphoreTake(isr_sem, pdMS_TO_TICKS(50));
//        if (rtos_result) {
//            syslog_printf("FUSB302 interrupt");
//            //fusb302_tcpc_alert(0);
//        }
        fusb302_tcpc_alert(0);
        pd_run_state_machine(0);
        if (dp_enabled && !hpd_sent && !pd_is_vdm_busy(0)) {
            syslog_printf("DP enabled\n");
            pd_send_hpd(0, hpd_high);
            hpd_sent = true;
        }
    }

    while (1) {
    	sleep_ms(100);
    }

    vSemaphoreDelete(isr_sem);
}
