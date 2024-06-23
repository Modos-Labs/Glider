//
// Copyright 2022 Wenting Zhang <zephray@outlook.com>
// Copyright 2017 Jason Cerundolo
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
#include "config.h"
#include "tcpm_driver.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "utils.h"

const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, FUSB302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};

void tcpc_i2c_init(void) {
    // Should be initialized at board level init to avoid dependencies between
    // drivers that need I2C
}

/* I2C wrapper functions - get I2C port / slave addr from config struct. */
int tcpc_write(int port, int reg, int val) {
    uint8_t buf[2];
    int result;
    buf[0] = (uint8_t)reg;
    buf[1] = (uint8_t)val;
    result = i2c_write_blocking(TCPC_I2C, FUSB302_I2C_SLAVE_ADDR, buf, 2, false);
    if (result != 2) {
        fatal("Failed writing data to TCPC");
    }

    return 0;
}

int tcpc_write16(int port, int reg, int val) {
    uint8_t buf[3];
    int result;
    buf[0] = (uint8_t)reg;
    buf[1] = (uint8_t)(val & 0xff);
    buf[2] = (uint8_t)((val >> 8) & 0xff);
    result = i2c_write_blocking(TCPC_I2C, FUSB302_I2C_SLAVE_ADDR, buf, 3, false);
    if (result != 3) {
        fatal("Failed writing data to TCPC");
    }
    
    return 0;
}

int tcpc_read(int port, int reg, int *val) {
    int result;
    uint8_t buf[1];
    buf[0] = reg;
    result = i2c_write_blocking(TCPC_I2C, FUSB302_I2C_SLAVE_ADDR, buf, 1, true);
    if (result != 1) {
        fatal("Failed writing data to TCPC");
    }
    result = i2c_read_blocking(TCPC_I2C, FUSB302_I2C_SLAVE_ADDR, buf, 1, false);
    if (result != 1) {
        fatal("Failed reading data from TCPC");
    }
    *val = (int)buf[0];

    return 0;
}

int tcpc_read16(int port, int reg, int *val) {
    uint8_t buf[2];
    int result;
    buf[0] = reg;
    result = i2c_write_blocking(TCPC_I2C, FUSB302_I2C_SLAVE_ADDR, buf, 1, true);
    if (result != 1) {
        fatal("Failed writing data to TCPC");
    }
    result = i2c_read_blocking(TCPC_I2C, FUSB302_I2C_SLAVE_ADDR, buf, 2, false);
    if (result != 2) {
        fatal("Failed reading data from TCPC");
    }
    *val = (int)buf[1] << 8 | buf[0];

    return 0;
}

int tcpc_xfer(int port,
        const uint8_t *out, int out_size,
        uint8_t *in, int in_size,
        int flags) {
    int result;
    if (out_size) {
        result = i2c_write_blocking(TCPC_I2C, FUSB302_I2C_SLAVE_ADDR, out, 
                out_size, !!(flags & I2C_XFER_STOP));
        if (result != out_size) {
            fatal("Failed writing data to TCPC");
        }
    }

    if (in_size) {
        result = i2c_read_blocking(TCPC_I2C, FUSB302_I2C_SLAVE_ADDR, in,
                in_size, !!(flags & I2C_XFER_STOP));
        if (result != in_size) {
            fatal("Failed reading data from TCPC");
        }
    }

    return 0;
}

