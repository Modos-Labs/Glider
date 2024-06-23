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
#include "pico/sleep.h"
#include "hardware/i2c.h"
#include "config.h"
#include "utils.h"
#include "tcpm_driver.h"
#include "usb_pd.h"
#include "ptn3460.h"
#include "adv7611.h"
#include "power.h"
#include "fpga.h"
#include "edid.h"
#include "button.h"
#include "caster.h"

int main()
{
    stdio_init_all();

    //sleep_ms(2000);

    printf("\n");
    printf("Glider\n");

#ifdef INPUT_ADV7611
    // Need to release ADV from reset, otherwise it would hold the I2C bus
    adv7611_early_init();
#endif

    // Initialize I2C for TCPC/PTN3460/ADV7611 use
    i2c_init(TCPC_I2C, 100*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

#ifdef HAS_TYPEC
    int result = tcpm_init(0);
    if (result)
        fatal("Failed to initialize TCPC\n");

    int cc1, cc2;
    tcpc_config[0].drv->get_cc(0, &cc1, &cc2);
    printf("CC status %d %d\n", cc1, cc2);
#endif

    power_init();

    pd_init(0);
    sleep_ms(50);

    edid_init();
#ifdef INPUT_PTN3460
    ptn3460_init();
#endif
#ifdef INPUT_ADV7611
    adv7611_init();
#endif

    //sleep_ms(1000);

    power_enable(true);

    //sleep_run_from_xosc();
    //sleep_goto_dormant_until_edge_high(8);
    // https://ghubcoder.github.io/posts/awaking-the-pico/

    fpga_init();
    caster_init();

    button_init();

    int mode_max = 6;
    int mode = 1;
    UPDATE_MODE modes[6] = {
        UM_FAST_MONO_NO_DITHER,
        UM_FAST_MONO_BAYER,
        UM_FAST_MONO_BLUE_NOISE,
        UM_FAST_GREY,
        UM_AUTO_LUT_NO_DITHER,
        UM_AUTO_LUT_ERROR_DIFFUSION
    };

    extern int dp_enabled;
    bool hpd_sent = false;
    bool dp_valid = false;

    while (1) {
#ifdef HAS_TYPEC
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
#endif

        // Key press logic
        uint32_t keys = button_scan();
        if (keys & 0x1) {
            // First key short press
            // Clear screen
            caster_redraw(0,0,2400,1800);
        }
        if (keys & 0x2) {
            // First key long press
        }
        if (keys & 0x4) {
            // Second key short press
             // Switch mode
            mode++;
            if (mode >= mode_max) mode = 0;
            caster_setmode(0,0,2400,1800,modes[mode]);
        }
        if (keys & 0x8) {
            // Second key long press
        }

        sleep_ms(1);
    }

    return 0;
}
