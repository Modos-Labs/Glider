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
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/sleep.h"
#include "hardware/i2c.h"
#include "config.h"
#include "utils.h"
#include "tcpm_driver.h"
#include "usb_pd.h"
#include "ptn3460.h"
#include "power.h"
#include "fpga.h"
#include "edid.h"
#include "caster.h"
#include "usbapp.h"

void osd_task(void); // OSD handling task
void usb_pd_task(void); // USB PD handling task

int main()
{
    stdio_init_all();

    //sleep_ms(2000);

    printf("\n");
    printf("Glider\n");

    power_init();

#ifdef INPUT_DVI
    edid_init();
#endif

#ifdef INPUT_TYPEC
    int result = tcpm_init(0);
    if (result)
        fatal("Failed to initialize TCPC\n");

    // int cc1, cc2;
    // tcpc_config[0].drv->get_cc(0, &cc1, &cc2);
    // printf("CC status %d %d\n", cc1, cc2);

    ptn3460_init();
    pd_init(0);
#endif

#ifdef BOARD_HAS_BUTTON
    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO);
#endif

    //fpga_init();
    //power_enable(true);

    usbapp_init();

    while (1) {
        osd_task();
        usb_pd_task();
        usbapp_task();
    }

    return 0;
}

void osd_task(void) {
#ifdef BOARD_HAS_BUTTON
    static int mode_max = 6;
    static int mode = 1;
    const UPDATE_MODE modes[6] = {
        UM_FAST_MONO_NO_DITHER,
        UM_FAST_MONO_BAYER,
        UM_FAST_MONO_BLUE_NOISE,
        UM_FAST_GREY,
        UM_AUTO_LUT_NO_DITHER,
        UM_AUTO_LUT_ERROR_DIFFUSION
    };

    if (gpio_get(BUTTON_GPIO) == 0) {
        sleep_ms(20);
        if (gpio_get(BUTTON_GPIO) == 0) {
            int i = 0;
            while (gpio_get(BUTTON_GPIO) == 0) {
                i++;
                sleep_ms(1);
                if (i > 500)
                    break;
            }
            if (i > 500) {
                // Long press, clear screen
                caster_redraw(0,0,1600,1200);
            }
            else {
                // Short press, switch mode
                mode++;
                if (mode >= mode_max) mode = 0;
                caster_setmode(0,0,1600,1200,modes[mode]);
            }
            while (gpio_get(BUTTON_GPIO) == 0);
        }
        while (gpio_get(BUTTON_GPIO) == 0);
    }
#endif
}

void usb_pd_task(void) {
    extern int dp_enabled;
    static bool hpd_sent = false;
    static bool dp_valid = false;

    // TODO: Implement interrupt
    fusb302_tcpc_alert(0);
    pd_run_state_machine(0);
    if (dp_enabled && !hpd_sent && !pd_is_vdm_busy(0)) {
        printf("DP enabled\n");
        pd_send_hpd(0, hpd_high);
        hpd_sent = true;
    }
    if (dp_valid != ptn3460_is_valid()) {
        dp_valid = ptn3460_is_valid();
        printf(dp_valid ? "Input is valid\n" : "Input is invalid\n");
    }
}
