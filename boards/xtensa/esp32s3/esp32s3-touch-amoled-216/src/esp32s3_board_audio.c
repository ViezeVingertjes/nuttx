/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-touch-amoled-216/src/esp32s3_board_audio.c
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
#include <nuttx/audio/audio.h>
#include <nuttx/audio/i2s.h>
#include <nuttx/audio/pcm.h>
#include <nuttx/i2c/i2c_master.h>

#ifdef CONFIG_AUDIO_ES8311
#  include <nuttx/audio/es8311.h>
#endif

#ifdef CONFIG_AUDIO_ES7210
#  include <nuttx/audio/es7210.h>
#endif

#include "espressif/esp_gpio.h"
#include "espressif/esp_i2s.h"
#include "esp32s3_i2c.h"

#include "esp32s3-touch-amoled-216.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ES8311_I2C_ADDR   0x18
#define ES8311_I2C_FREQ   100000

#define ES7210_I2C_ADDR   0x40
#define ES7210_I2C_FREQ   100000

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_AUDIO_ES8311
static const struct es8311_lower_s g_es8311_lower =
{
  .frequency = ES8311_I2C_FREQ,
  .address   = ES8311_I2C_ADDR,
};
#endif

#ifdef CONFIG_AUDIO_ES7210
static const struct es7210_lower_s g_es7210_lower =
{
  .frequency = ES7210_I2C_FREQ,
  .address   = ES7210_I2C_ADDR,
};
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_audio_initialize
 *
 * Description:
 *   Register the audio devices.  Both codecs share I2S0: the ES8311 drives
 *   the speaker amplifier from the I2S data output and is registered as
 *   /dev/audio/pcm0; the ES7210 digitizes the two microphones onto the I2S
 *   data input and is registered as /dev/audio/pcm_in0.  The ES8311 ADC is
 *   not wired to anything on this board, so it is not registered.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_audio_initialize(void)
{
  FAR struct audio_lowerhalf_s *lower;
  FAR struct i2c_master_s *i2c;
  FAR struct i2s_dev_s *i2s;
  int ret;

  i2c = esp32s3_i2cbus_initialize(BOARD_I2C_BUS);
  if (i2c == NULL)
    {
      return -ENODEV;
    }

  i2s = esp_i2sbus_initialize(ESP32S3_I2S0);
  if (i2s == NULL)
    {
      esp32s3_i2cbus_uninitialize(i2c);
      return -ENODEV;
    }

  /* The speaker power amplifier is only switched on when explicitly
   * enabled in the board configuration; otherwise it is held off.
   */

  esp_configgpio(GPIO_SPK_PA_EN, OUTPUT);
#ifdef CONFIG_ESP32S3_TOUCH_AMOLED_216_SPEAKER
  esp_gpiowrite(GPIO_SPK_PA_EN, true);
#else
  esp_gpiowrite(GPIO_SPK_PA_EN, false);
#endif

#ifdef CONFIG_AUDIO_ES8311

  lower = es8311_initialize(i2c, i2s, &g_es8311_lower);
  if (lower == NULL)
    {
      auderr("ERROR: Failed to initialize the ES8311\n");
      return -ENODEV;
    }

#  ifndef CONFIG_SYSTEM_NXLOOPER
  /* Put a PCM decoder in front of the codec, so that WAV files can be
   * played.  nxlooper feeds raw PCM buffers and does not need it.
   */

  lower = pcm_decode_initialize(lower);
  if (lower == NULL)
    {
      auderr("ERROR: Failed to create the PCM decoder\n");
      return -ENODEV;
    }
#  endif

  ret = audio_register("pcm0", lower);
  if (ret < 0)
    {
      auderr("ERROR: Failed to register pcm0: %d\n", ret);
      return ret;
    }
#endif

#ifdef CONFIG_AUDIO_ES7210
  lower = es7210_initialize(i2c, i2s, &g_es7210_lower);
  if (lower == NULL)
    {
      auderr("ERROR: Failed to initialize the ES7210\n");
      return -ENODEV;
    }

  ret = audio_register("pcm_in0", lower);
  if (ret < 0)
    {
      auderr("ERROR: Failed to register pcm_in0: %d\n", ret);
      return ret;
    }
#endif

  UNUSED(lower);
  UNUSED(ret);
  return OK;
}
