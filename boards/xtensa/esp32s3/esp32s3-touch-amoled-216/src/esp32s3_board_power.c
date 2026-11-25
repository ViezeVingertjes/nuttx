/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-touch-amoled-216/src/esp32s3_board_power.c
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

#include <nuttx/board.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/power/axp2101.h>
#include <nuttx/power/battery_charger.h>
#include <nuttx/power/battery_gauge.h>

#include "esp32s3_i2c.h"

#include "esp32s3-touch-amoled-216.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AXP2101_I2C_FREQ  400000

#ifndef CONFIG_ESP32S3_I2C0
#  error "The AXP2101 needs CONFIG_ESP32S3_I2C0 (SDA GPIO15, SCL GPIO14)"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct battery_charger_dev_s *g_axp2101;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_axp2101_initialize
 *
 * Description:
 *   Register the AXP2101 battery charger as /dev/batt0 and, if enabled,
 *   its fuel gauge as /dev/gauge0.  The power rails are left as the
 *   AXP2101 OTP configured them.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_axp2101_initialize(void)
{
  FAR struct i2c_master_s *i2c;
#ifdef CONFIG_AXP2101_GAUGE
  FAR struct battery_gauge_dev_s *gauge;
#endif
  int ret;

  i2c = esp32s3_i2cbus_initialize(BOARD_I2C_BUS);
  if (i2c == NULL)
    {
      return -ENODEV;
    }

  g_axp2101 = axp2101_initialize(i2c, AXP2101_I2C_ADDR, AXP2101_I2C_FREQ);
  if (g_axp2101 == NULL)
    {
      esp32s3_i2cbus_uninitialize(i2c);
      return -ENODEV;
    }

  ret = battery_charger_register("/dev/batt0", g_axp2101);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_AXP2101_GAUGE
  gauge = axp2101_gauge_initialize(i2c, AXP2101_I2C_ADDR, AXP2101_I2C_FREQ);
  if (gauge == NULL)
    {
      return -ENODEV;
    }

  ret = battery_gauge_register("/dev/gauge0", gauge);
  if (ret < 0)
    {
      return ret;
    }
#endif

  return OK;
}

/****************************************************************************
 * Name: board_power_off
 *
 * Description:
 *   Power the board off through the AXP2101.  The PWR key turns it back
 *   on.
 *
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_POWEROFF
int board_power_off(int status)
{
  UNUSED(status);

  if (g_axp2101 == NULL)
    {
      return -ENODEV;
    }

  return axp2101_poweroff(g_axp2101);
}
#endif
