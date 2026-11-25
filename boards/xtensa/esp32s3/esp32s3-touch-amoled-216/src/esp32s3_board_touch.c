/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-touch-amoled-216/src/esp32s3_board_touch.c
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
#include <stdbool.h>

#include <nuttx/debug.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/cst92xx.h>
#include <nuttx/input/touchscreen.h>

#include "espressif/esp_gpio.h"
#include "esp32s3_i2c.h"

#include "esp32s3-touch-amoled-216.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int tp_attach(FAR const struct cst92xx_config_s *config,
                     xcpt_t isr, FAR void *arg);
static void tp_enable(FAR const struct cst92xx_config_s *config,
                      bool enable);
static void tp_reset(FAR const struct cst92xx_config_s *config,
                     bool assert);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The panel is mounted so that the controller's native axes have to be
 * swapped and X mirrored to match the display orientation.
 */

static const struct cst92xx_config_s g_cst92xx_config =
{
  .address   = CST92XX_I2C_ADDRESS,
  .frequency = 400000,
  .xres      = 480,
  .yres      = 480,
  .flags     = TOUCH_FLAG_SWAPXY | TOUCH_FLAG_MIRRORX,
  .attach    = tp_attach,
  .enable    = tp_enable,
  .reset     = tp_reset,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int tp_attach(FAR const struct cst92xx_config_s *config,
                     xcpt_t isr, FAR void *arg)
{
  return esp_gpio_irq(GPIO_TP_INT, isr, arg);
}

static void tp_enable(FAR const struct cst92xx_config_s *config,
                      bool enable)
{
  if (enable)
    {
      esp_gpioirqenable(GPIO_TP_INT);
    }
  else
    {
      esp_gpioirqdisable(GPIO_TP_INT);
    }
}

static void tp_reset(FAR const struct cst92xx_config_s *config,
                     bool assert)
{
  /* TP_RST is active low */

  esp_gpiowrite(GPIO_TP_RST, !assert);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_touchscreen_initialize
 *
 * Description:
 *   Register the CST9220 touch controller as /dev/input0.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_touchscreen_initialize(void)
{
  FAR struct i2c_master_s *i2c;
  int ret;

  esp_configgpio(GPIO_TP_RST, OUTPUT);
  esp_gpiowrite(GPIO_TP_RST, true);
  esp_configgpio(GPIO_TP_INT, INPUT_PULLUP | FALLING);

  i2c = esp32s3_i2cbus_initialize(BOARD_I2C_BUS);
  if (i2c == NULL)
    {
      return -ENODEV;
    }

  ret = cst92xx_register(i2c, &g_cst92xx_config, 0);
  if (ret < 0)
    {
      ierr("ERROR: cst92xx_register() failed: %d\n", ret);
      esp32s3_i2cbus_uninitialize(i2c);
    }

  return ret;
}
