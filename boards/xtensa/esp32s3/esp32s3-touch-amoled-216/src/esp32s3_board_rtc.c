/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-touch-amoled-216/src/esp32s3_board_rtc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <syslog.h>

#include <nuttx/clock.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/timers/pcf85063.h>

#include "esp32s3_i2c.h"

#include "esp32s3-touch-amoled-216.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The PCF85063 driver provides the up_rtc_* interface itself, which the
 * ESP32-S3 internal RTC driver (CONFIG_RTC_DRIVER) also provides.
 */

#ifdef CONFIG_RTC_DRIVER
#  error "CONFIG_RTC_PCF85063 cannot be used together with CONFIG_RTC_DRIVER"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_rtc_initialize
 *
 * Description:
 *   Called by clock_initialize() very early during boot, before the I2C
 *   bus is available.  The PCF85063 is brought up later by
 *   board_pcf85063_initialize(), which then synchronizes the system time.
 *
 ****************************************************************************/

int up_rtc_initialize(void)
{
  return OK;
}

/****************************************************************************
 * Name: board_pcf85063_initialize
 *
 * Description:
 *   Bring up the PCF85063 on the shared I2C bus, use it as the external
 *   RTC and load the system time from it.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_pcf85063_initialize(void)
{
  FAR struct i2c_master_s *i2c;
  int ret;

  i2c = esp32s3_i2cbus_initialize(BOARD_I2C_BUS);
  if (i2c == NULL)
    {
      return -ENODEV;
    }

  ret = pcf85063_rtc_initialize(i2c);
  if (ret < 0)
    {
      esp32s3_i2cbus_uninitialize(i2c);
      return ret;
    }

  /* Now that the RTC is available, load the system time from it */

  clock_synchronize(NULL);
  return OK;
}
