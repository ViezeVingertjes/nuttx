# ##############################################################################
# boards/xtensa/esp32s3/esp32s3-touch-amoled-216/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

set(SRCS esp32s3_boot.c esp32s3_bringup.c)

if(CONFIG_BOARDCTL_RESET)
  list(APPEND SRCS esp32s3_reset.c)
endif()

if(CONFIG_ESP32S3_SPI)
  list(APPEND SRCS esp32s3_board_spi.c)
endif()

if(CONFIG_RTC_PCF85063)
  list(APPEND SRCS esp32s3_board_rtc.c)
endif()

if(CONFIG_INPUT_CST92XX)
  list(APPEND SRCS esp32s3_board_touch.c)
endif()

if(CONFIG_AXP2101)
  list(APPEND SRCS esp32s3_board_power.c)
endif()

if(CONFIG_LCD_CO5300)
  list(APPEND SRCS esp32s3_board_lcd.c)
endif()

if(CONFIG_AUDIO_ES8311 OR CONFIG_AUDIO_ES7210)
  list(APPEND SRCS esp32s3_board_audio.c)
endif()

if(CONFIG_ARCH_BUTTONS)
  list(APPEND SRCS esp32s3_buttons.c)
endif()

target_sources(board PRIVATE ${SRCS})
