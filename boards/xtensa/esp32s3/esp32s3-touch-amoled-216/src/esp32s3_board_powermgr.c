/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-touch-amoled-216/src/esp32s3_board_powermgr.c
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

#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>

#include <nuttx/board.h>
#include <nuttx/clock.h>
#include <nuttx/wqueue.h>
#include <nuttx/fs/fs.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/power/battery_charger.h>
#include <nuttx/power/battery_gauge.h>

#include "esp32s3-touch-amoled-216.h"

#ifdef CONFIG_ESP32S3_TOUCH_AMOLED_216_POWERMGR

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_LCD_CO5300) && \
    CONFIG_ESP32S3_TOUCH_AMOLED_216_SCREEN_TIMEOUT > 0
#  define HAVE_SCREEN_TIMEOUT 1
#  define SCREEN_TIMEOUT \
     SEC2TICK(CONFIG_ESP32S3_TOUCH_AMOLED_216_SCREEN_TIMEOUT)
#endif

#ifdef CONFIG_AXP2101_GAUGE
#  define HAVE_BATTERY_MONITOR 1
#  define BATT_INTERVAL \
     SEC2TICK(CONFIG_ESP32S3_TOUCH_AMOLED_216_BATT_INTERVAL)
#  ifdef CONFIG_ESP32S3_TOUCH_AMOLED_216_BATT_OFF
#    define BATT_OFF    CONFIG_ESP32S3_TOUCH_AMOLED_216_BATT_OFF
#    define BATT_OFF_MV CONFIG_ESP32S3_TOUCH_AMOLED_216_BATT_OFF_MV
#  else
#    define BATT_OFF    0
#    define BATT_OFF_MV 0
#  endif
#  define BATT_WARN     CONFIG_ESP32S3_TOUCH_AMOLED_216_BATT_WARN
#  define BATT_WARN_MV  CONFIG_ESP32S3_TOUCH_AMOLED_216_BATT_WARN_MV

/* The warning is given again once the charge and the voltage have risen
 * this much above the warning levels (or the board is on USB power).
 */

#  define BATT_HYSTERESIS    2
#  define BATT_HYSTERESIS_MV 100
#endif

/* Only apply the display timeout on battery if so configured */

#if defined(HAVE_SCREEN_TIMEOUT) && defined(CONFIG_AXP2101) && \
    !defined(CONFIG_ESP32S3_TOUCH_AMOLED_216_SCREEN_TIMEOUT_USB)
#  define SCREEN_BATTERY_ONLY 1
#endif

#if defined(SCREEN_BATTERY_ONLY) || defined(HAVE_BATTERY_MONITOR)
#  define NEED_ON_USB 1
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef HAVE_SCREEN_TIMEOUT
static FAR struct lcd_dev_s *g_lcd;
static struct work_s g_screenwork;
static volatile clock_t g_lastactivity;
static volatile bool g_screenoff;
static int g_screenpower;
#  ifdef CONFIG_INPUT_CST92XX
static struct file g_touchfile;
#  endif
#endif

#ifdef HAVE_BATTERY_MONITOR
static struct work_s g_battwork;
static bool g_battwarned;
static int g_battlow;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef NEED_ON_USB
/****************************************************************************
 * Name: powermgr_on_usb
 *
 * Description:
 *   Return true if the board is powered by USB (VBUS present).  If that
 *   cannot be read, USB is assumed, so that the board is never powered
 *   off by mistake.
 *
 ****************************************************************************/

static bool powermgr_on_usb(void)
{
  FAR struct battery_charger_dev_s *charger = board_axp2101_charger();
  bool online = true;

  if (charger == NULL || charger->ops->online == NULL ||
      charger->ops->online(charger, &online) < 0)
    {
      online = true;
    }

  return online;
}
#endif

#ifdef HAVE_SCREEN_TIMEOUT
/****************************************************************************
 * Name: powermgr_screen
 *
 * Description:
 *   Turn the display on after activity, or off once the timeout has
 *   expired without activity.  Runs on the low priority work queue, as
 *   the LCD driver may not be called from interrupt handlers.
 *
 ****************************************************************************/

static void powermgr_screen(FAR void *arg)
{
  clock_t elapsed;

  if (g_screenoff)
    {
      g_lcd->setpower(g_lcd, g_screenpower);
      g_screenoff = false;
      work_queue(LPWORK, &g_screenwork, powermgr_screen, NULL,
                 SCREEN_TIMEOUT);
      return;
    }

  elapsed = clock_systime_ticks() - g_lastactivity;
  if (elapsed < SCREEN_TIMEOUT)
    {
      work_queue(LPWORK, &g_screenwork, powermgr_screen, NULL,
                 SCREEN_TIMEOUT - elapsed);
      return;
    }

#  ifdef SCREEN_BATTERY_ONLY
  if (powermgr_on_usb())
    {
      work_queue(LPWORK, &g_screenwork, powermgr_screen, NULL,
                 SCREEN_TIMEOUT);
      return;
    }
#  endif

  g_screenpower = g_lcd->getpower(g_lcd);
  if (g_screenpower <= 0)
    {
      g_screenpower = CONFIG_LCD_MAXPOWER;
    }

  g_screenoff = true;
  g_lcd->setpower(g_lcd, 0);
}
#endif

#ifdef HAVE_BATTERY_MONITOR
/****************************************************************************
 * Name: powermgr_battery
 *
 * Description:
 *   Check the battery charge while running on battery: warn when it is
 *   low, and flush the file systems and power off when it is nearly
 *   empty.
 *
 ****************************************************************************/

static void powermgr_battery(FAR void *arg)
{
  FAR struct battery_gauge_dev_s *gauge = board_axp2101_gauge();
  bool havecap;
  bool havemv;
  bool empty;
  bool low;
  int capacity;
  int mv;

  if (gauge == NULL || powermgr_on_usb())
    {
      g_battwarned = false;
      g_battlow    = 0;
      goto out;
    }

  /* The protection decisions use the battery voltage as well as the charge
   * estimated by the fuel gauge, so that they do not depend on the battery
   * model of the gauge alone.  If one of them cannot be read, the other
   * one decides.
   */

  havecap = gauge->ops->capacity(gauge, &capacity) >= 0;
  havemv  = gauge->ops->voltage(gauge, &mv) >= 0;
  if (!havecap && !havemv)
    {
      goto out;
    }

  empty = (havecap && BATT_OFF > 0 && capacity <= BATT_OFF) ||
          (havemv && BATT_OFF_MV > 0 && mv <= BATT_OFF_MV);
  low   = (havecap && BATT_WARN > 0 && capacity <= BATT_WARN) ||
          (havemv && BATT_WARN_MV > 0 && mv <= BATT_WARN_MV);

  /* Require two low readings in a row before powering off, so that a
   * single glitch cannot turn the board off.
   */

  if (empty)
    {
      if (++g_battlow >= 2)
        {
          syslog(LOG_EMERG, "Battery empty (%d%%, %d mV): powering off\n",
                 havecap ? capacity : -1, havemv ? mv : -1);
          sync();
#  ifdef CONFIG_BOARDCTL_POWEROFF
          board_power_off(0);
#  endif
        }
    }
  else
    {
      g_battlow = 0;
    }

  if (low && !g_battwarned)
    {
      syslog(LOG_WARNING, "Battery low: %d%%, %d mV\n",
             havecap ? capacity : -1, havemv ? mv : -1);
      g_battwarned = true;
    }
  else if ((!havecap || capacity > BATT_WARN + BATT_HYSTERESIS) &&
           (!havemv || mv > BATT_WARN_MV + BATT_HYSTERESIS_MV))
    {
      g_battwarned = false;
    }

out:
  work_queue(LPWORK, &g_battwork, powermgr_battery, NULL, BATT_INTERVAL);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_powermgr_activity
 *
 * Description:
 *   Report user activity (touch or key).  Called from the touch and key
 *   interrupt handlers: it only records the time and, if the display is
 *   off, queues the work that turns it on.
 *
 ****************************************************************************/

void board_powermgr_activity(void)
{
#ifdef HAVE_SCREEN_TIMEOUT
  g_lastactivity = clock_systime_ticks();

  if (g_screenoff && g_lcd != NULL && work_available(&g_screenwork))
    {
      work_queue(LPWORK, &g_screenwork, powermgr_screen, NULL, 0);
    }
#endif
}

/****************************************************************************
 * Name: board_powermgr_initialize
 *
 * Description:
 *   Start the power manager.  Must be called after the LCD, touch and
 *   AXP2101 devices have been initialized.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_powermgr_initialize(void)
{
  int ret = OK;

#ifdef HAVE_SCREEN_TIMEOUT
  g_lcd = board_lcd_getdev(0);
  if (g_lcd != NULL)
    {
      g_screenpower  = CONFIG_LCD_MAXPOWER;
      g_lastactivity = clock_systime_ticks();

#  ifdef CONFIG_INPUT_CST92XX
      /* Keep the touch controller active, so that a touch wakes the
       * display even if no application has the touch device open.
       */

      ret = file_open(&g_touchfile, "/dev/input0", O_RDONLY | O_NONBLOCK);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: powermgr: cannot open touch: %d\n", ret);
        }
#  endif

#  ifdef CONFIG_ESP32S3_TOUCH_AMOLED_216_POWERMGR_KEYS
      /* Report key presses as activity as well */

      board_buttons_watch();
#  endif

      work_queue(LPWORK, &g_screenwork, powermgr_screen, NULL,
                 SCREEN_TIMEOUT);
    }
#endif

#ifdef HAVE_BATTERY_MONITOR
  work_queue(LPWORK, &g_battwork, powermgr_battery, NULL, BATT_INTERVAL);
#endif

  return ret;
}

#endif /* CONFIG_ESP32S3_TOUCH_AMOLED_216_POWERMGR */
