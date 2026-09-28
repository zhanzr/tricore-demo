/*
  touch.h - CST816D capacitive touch over bit-banged I2C (SDA = P02.0,
  SCL = P02.1) for the appkit-tc234 TK018F3716 module port.

  The touch data block is read from register 0x00 (the vendor example
  reads 8 bytes from 0; registers: 0x00 Device_Mode, 0x01 GEST_ID,
  0x02 TD_STATUS, 0x03 P1_XH, 0x04 P1_XL, 0x05 P1_YH, 0x06 P1_YL).
  7-bit slave address 0x15 (the byte on the wire is 0x2A for write).
*/

#ifndef __TOUCH_H
#define __TOUCH_H

#include <stdint.h>

void Touch_Init(void);
void Touch_Read(uint8_t *buf, uint8_t len);

/* Bring-up check: probe the bus and return 1 if the CST816D ACKs its
 * address, 0 otherwise. Distinguishes "no finger down" (valid data,
 * buf[3] != 0x80) from "the chip never answered" (wiring/address). */
uint8_t Touch_SelfTest(void);
void    Touch_Scan(void);

/* Bring-up pin hunt: try every plausible free X700 pin pair at runtime and
 * report where the touch chip answers. Returns 1 on success (and leaves the
 * bus pointed at the working pair). One flash covers all candidate wiring,
 * so a wrong pin guess does not need another flash cycle. */
uint8_t Touch_HuntPins(void);

/* Wiring fact, not a protocol test: enable the INTERNAL pull-down on each
 * candidate pin and read it. A pin that still reads 1 has an EXTERNAL
 * pull-up, i.e. it is really sitting on an I2C bus. Independent of I2C
 * timing, so it cannot be masked by a marginal clock or a bad ACK phase. */
void Touch_HuntPullups(void);

/* Configuration figure: the SCL rate actually asked for. The real rate is
 * quantised by the bit-bang delay's tick granularity, so this reports the
 * request, not a derived value. */
uint32_t Touch_GetHz(void);

#endif /* __TOUCH_H */
