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
#include "platform.h"
#include "board.h"
#include "app.h"
#include "fusb302.h"

const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
  {0, FUSB302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};

void tcpc_i2c_init(void) {
    // Should be initialized at board level init to avoid dependencies between
    // drivers that need I2C
}

/* I2C wrapper functions - get I2C port / slave addr from config struct. */
int tcpc_write(int port, int reg, int val) {
    int result = pal_i2c_write_reg(FUSB302_I2C, FUSB302_I2C_SLAVE_ADDR, reg, val);
    if (result != 0) {
        syslog_print("Failed writing 8b reg to TCPC");
        while (1) {
			sleep_ms(500);
		}
    }
    return 0;
}

int tcpc_write16(int port, int reg, int val) {
    uint8_t buf[2];
    int result;
    buf[0] = (uint8_t)(val & 0xff);
    buf[1] = (uint8_t)((val >> 8) & 0xff);
    result = pal_i2c_write_longreg(FUSB302_I2C, FUSB302_I2C_SLAVE_ADDR, reg, buf, 2);
    if (result != 0) {
        syslog_print("Failed writing 16b reg to TCPC");
        while (1) {
			sleep_ms(500);
		}
    }
    return 0;
}

int tcpc_read(int port, int reg, int *val) {
	uint8_t rd;
    int result = pal_i2c_read_reg(FUSB302_I2C, FUSB302_I2C_SLAVE_ADDR, reg, &rd);
    *val = rd;
    if (result != 0) {
        syslog_print("Failed reading register from TCPC");
        while (1) {
        	sleep_ms(500);
        }
    }
    return 0;
}

int tcpc_read16(int port, int reg, int *val) {
    uint8_t buf[2];
    int result;

    uint8_t wr = reg;
    result = pal_i2c_read_payload(FUSB302_I2C, FUSB302_I2C_SLAVE_ADDR, &wr, 1, buf, 2);
    if (result != 0) {
        syslog_print("Failed writing data to TCPC");
        while (1) {
			sleep_ms(500);
		}
    }
    *val = (int)buf[1] << 8 | buf[0];

    return 0;
}

int tcpc_xfer(int port,
        const uint8_t *out, int out_size,
        uint8_t *in, int in_size,
        int flags) {
    static bool xfer_in_progress = false;

    if (!xfer_in_progress) {
        // Start from idle
        pal_i2c_ll_lock(FUSB302_I2C);
    }

    // Write
    // Write mode:
    // if start requested: use REQ_WRITE
    // otherwise: use REQ_NONE
    // Read
    // Read mode:
    // if start requested: if already in transaction, issue repeated start, otherwise issue start
    // otherwise: use REQ_NONE

    if (out_size != 0) {
        uint32_t request = (flags & I2C_XFER_START) ? REQ_WRITE : REQ_NONE;
        xfer_in_progress = true;
        if (!pal_i2c_ll_start(FUSB302_I2C, request, FUSB302_I2C_SLAVE_ADDR << 1, out_size))
            goto fail;
        for (int i = 0; i < out_size; i++) {
            if (!pal_i2c_ll_send(FUSB302_I2C, out[i])) {
            	//0x0046syslog_printf("Byte %d", i);
                goto fail;
            }
        }
    }

    if (in_size != 0) {
        uint32_t request = (flags & I2C_XFER_START) ?
            (xfer_in_progress ? REQ_RESTART_READ : REQ_READ) : REQ_NONE;
        if (!(flags & I2C_XFER_STOP)) {
        	// Would followed by another read
        	request |= REQ_RELOAD;
        }
        xfer_in_progress = true;
        if (!pal_i2c_ll_start(FUSB302_I2C, request, FUSB302_I2C_SLAVE_ADDR << 1, in_size))
            goto fail;
        for (int i = 0; i < in_size; i++) {
            if (!pal_i2c_ll_recv(FUSB302_I2C, &in[i]))
                goto fail;
        }
    }

    if (flags & I2C_XFER_STOP) {
        goto done;
    }

    return 0;

fail:
    syslog_print("Failed transfer data from/ to TCPC");
    syslog_printf("OUT %d IN %d FLAGS %s%s", out_size, in_size, flags & I2C_XFER_START ? "START " : " ", flags & I2C_XFER_STOP ? "STOP" : "");
done:
    xfer_in_progress = false;
    pal_i2c_ll_stop(FUSB302_I2C);
    pal_i2c_ll_unlock(FUSB302_I2C);
    return 0;
}

