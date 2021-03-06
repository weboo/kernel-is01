/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __LINUX_I2C_TPS65023_H
#define __LINUX_I2C_TPS65023_H

#ifndef CONFIG_TPS65023
/* Set the output voltage for the DCDC1 convertor */
#define tps65023_set_dcdc1_level(mvolts)  (-ENODEV)

/* Read the output voltage from the DCDC1 convertor */
#define tps65023_get_dcdc1_level(mvolts)  (-ENODEV)

/* 2009.12.28 DCDC3 control function */
#define tps65023_dcdc3_control(onoff)     (-ENODEV)

#else
/* Set the output voltage for the DCDC1 convertor */
extern int tps65023_set_dcdc1_level(int mvolts);

/* Read the output voltage from the DCDC1 convertor */
extern int tps65023_get_dcdc1_level(int *mvolts);

/* 2009.12.28 DCDC3 control function */
#define SH_CAMERA_USER  0x01
#define SH_WLAN_USER    0x02

#define SH_PM_DCDC3_ON  1
#define SH_PM_DCDC3_OFF 0
extern int tps65023_dcdc3_control(int onoff, int user);

#endif

#endif
