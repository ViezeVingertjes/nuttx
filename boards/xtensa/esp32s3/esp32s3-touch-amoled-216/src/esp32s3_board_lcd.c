/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-touch-amoled-216/src/esp32s3_board_lcd.c
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

#include <nuttx/board.h>
#include <nuttx/signal.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/co5300.h>
#include <nuttx/spi/qspi.h>

#include "espressif/esp_gpio.h"
#include "esp32s3_qspi.h"

#include "esp32s3-touch-amoled-216.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct lcd_dev_s *g_lcd;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_lcd_initialize
 *
 * Description:
 *   Reset the CO5300 AMOLED panel, bring up the QSPI bus on SPI2 and
 *   initialize the panel.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_lcd_initialize(void)
{
  struct qspi_dev_s *qspi;
  int ret;

  if (g_lcd != NULL)
    {
      return OK;
    }

  /* Hardware reset of the panel (active low) */

  esp_configgpio(GPIO_LCD_RST, OUTPUT);
  esp_gpiowrite(GPIO_LCD_RST, true);
  nxsig_usleep(10000);
  esp_gpiowrite(GPIO_LCD_RST, false);
  nxsig_usleep(10000);
  esp_gpiowrite(GPIO_LCD_RST, true);
  nxsig_usleep(150000);

  qspi = esp32s3_qspibus_initialize(ESP32S3_SPI2);
  if (qspi == NULL)
    {
      return -ENODEV;
    }

  /* Commands and their parameters go out on a single line; the driver
   * switches to four lines for pixel data on its own.
   */

  ret = esp32s3_qspibus_set_attr(qspi, 0, 1, 1);
  if (ret < 0)
    {
      goto errout;
    }

  g_lcd = co5300_lcdinitialize(qspi);
  if (g_lcd == NULL)
    {
      ret = -ENODEV;
      goto errout;
    }

  return OK;

errout:
  esp32s3_qspibus_uninitialize(qspi);
  return ret;
}

/****************************************************************************
 * Name: board_lcd_getdev
 *
 * Description:
 *   Return a reference to the LCD object for the specified LCD.
 *
 ****************************************************************************/

struct lcd_dev_s *board_lcd_getdev(int devno)
{
  return devno == 0 ? g_lcd : NULL;
}

/****************************************************************************
 * Name: board_lcd_uninitialize
 *
 * Description:
 *   Uninitialize the LCD support.  Nothing to do, the panel stays on.
 *
 ****************************************************************************/

void board_lcd_uninitialize(void)
{
}
