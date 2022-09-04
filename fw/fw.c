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
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "utils.h"
#include "tcpm_driver.h"
#include "usb_pd.h"
#include "ptn3460.h"

int main()
{
    stdio_init_all();

    sleep_ms(1000);

    int result = tcpm_init(0);
    if (result)
        fatal("Failed to initialize TCPC\n");

    int cc1, cc2;
    tcpc_config[0].drv->get_cc(0, &cc1, &cc2);
    printf("CC status %d %d\n", cc1, cc2);

    ptn3460_init();
    pd_init(0);
    sleep_ms(50);

    extern int dp_enabled;
    bool hpd_sent = false;

    while (1) {
        // TODO: Implement interrupt
        fusb302_tcpc_alert(0);
        pd_run_state_machine(0);
        if (dp_enabled && !hpd_sent && !pd_is_vdm_busy(0)) {
            printf("DP enabled\n");
            pd_send_hpd(0, hpd_high);
            hpd_sent = true;
        }
    }

    return 0;
}
