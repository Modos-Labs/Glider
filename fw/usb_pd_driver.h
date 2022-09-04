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
#ifndef USB_PD_DRIVER_H_
#define USB_PD_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_pd.h"

#include <stdint.h>

//#define CONFIG_BBRAM
//#define CONFIG_CHARGE_MANAGER
//#define CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP
//#define CONFIG_USBC_VCONN_SWAP
#define CONFIG_USB_PD_ALT_MODE
//#define CONFIG_USB_PD_CHROMEOS
#define CONFIG_USB_PD_DUAL_ROLE
//#define CONFIG_USB_PD_GIVE_BACK
//#define CONFIG_USB_PD_SIMPLE_DFP
//#define CONFIG_USB_PD_TCPM_TCPCI

/* USB configuration */
#define CONFIG_USB_PID 0x500c
#define CONFIG_USB_BCD_DEV 0x0001 /* v 0.01 */

#define CONFIG_USB_PD_IDENTITY_HW_VERS 1
#define CONFIG_USB_PD_IDENTITY_SW_VERS 1

/* Default pull-up value on the USB-C ports when they are used as source. */
#define CONFIG_USB_PD_PULLUP TYPEC_RP_USB

/* Override PD_ROLE_DEFAULT in usb_pd.h */
#define PD_ROLE_DEFAULT(port) (PD_ROLE_SINK)

/* Don't automatically change roles */
#undef CONFIG_USB_PD_INITIAL_DRP_STATE
#define CONFIG_USB_PD_INITIAL_DRP_STATE PD_DRP_FREEZE

/* board specific type-C power constants */
/*
 * delay to turn on the power supply max is ~16ms.
 * delay to turn off the power supply max is about ~180ms.
 */
#define PD_POWER_SUPPLY_TURN_ON_DELAY  10000  /* us */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY 20000 /* us */

/* Define typical operating power and max power */
#define PD_OPERATING_POWER_MW (2250ull)
#define PD_MAX_POWER_MW       (15000ull)
#define PD_MAX_CURRENT_MA     (3000ull)
#define PD_MAX_VOLTAGE_MV     (5000ull)

#define PDO_FIXED_FLAGS (PDO_FIXED_COMM_CAP)

#define usleep(us) (delay_us(us))
#define msleep(ms) (delay_ms(ms))

typedef union {
	uint64_t val;
	struct {
		uint32_t lo;
		uint32_t hi;
		} le /* little endian words */;
	} timestamp_t;

uint32_t pd_task_set_event(uint32_t event, int wait_for_reply);
void pd_power_supply_reset(int port);

// Get the current timestamp from the system timer.
timestamp_t get_time(void);

/* Standard macros / definitions */
#ifndef MAX
#define MAX(a, b)					\
({						\
	__typeof__(a) temp_a = (a);		\
	__typeof__(b) temp_b = (b);		\
	\
	temp_a > temp_b ? temp_a : temp_b;	\
})
#endif
#ifndef MIN
#define MIN(a, b)					\
({						\
	__typeof__(a) temp_a = (a);		\
	__typeof__(b) temp_b = (b);		\
	\
	temp_a < temp_b ? temp_a : temp_b;	\
})
#endif

#ifdef __cplusplus
}
#endif

#endif /* USB_PD_DRIVER_H_ */
