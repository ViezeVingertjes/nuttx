====================================
Waveshare ESP32-S3-Touch-AMOLED-2.16
====================================

.. tags:: arch:xtensa, chip:esp32s3, vendor:espressif

This page discusses issues unique to NuttX configurations for the
Waveshare ESP32-S3-Touch-AMOLED-2.16 board.

.. figure:: esp32s3-touch-amoled-216.jpg
   :align: center

   ESP32-S3-Touch-AMOLED-2.16 (image: Waveshare, Apache-2.0)

The ESP32-S3-Touch-AMOLED-2.16 is a compact development board with a
2.16 inch square AMOLED touch display, power management with a Li-ion
battery charger, motion sensing, a real time clock, audio input and output
and a microSD card slot.

Features
========

  - ESP32-S3R8, dual core Xtensa LX7 at up to 240 MHz, with 8MB octal PSRAM
  - 16MB quad SPI flash
  - 2.16 inch 480x480 AMOLED, CO5300 controller on a QSPI bus
  - CST9220 capacitive touch controller
  - AXP2101 power management IC with Li-ion battery charger
  - QMI8658 6-axis IMU (accelerometer and gyroscope)
  - PCF85063 real time clock with backup supply
  - ES8311 audio codec driving a speaker amplifier
  - ES7210 four channel audio ADC with two microphones
  - microSD card slot
  - BOOT, PWR and one user (IO18) key
  - USB Type-C connected to the ESP32-S3 USB Serial/JTAG controller

Board documentation: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-2.16

Schematic and examples: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-2.16

Serial Console
==============

The board has no USB to UART bridge.  The USB Type-C connector goes to the
ESP32-S3 USB Serial/JTAG controller, which is used both to flash the board
and as the NuttShell console (``/dev/console``, enumerated by the host as
``/dev/ttyACM0`` on Linux).  UART0 (GPIO43 TX, GPIO44 RX) is available on
the side header.

Board Buttons
=============

The board has three keys:

  ====== ======== ====================================================
  Key    GPIO     Description
  ====== ======== ====================================================
  BOOT   GPIO0    Active low. Held at reset, enters the ROM bootloader
  IO18   GPIO18   Active low, free for the application
  PWR    GPIO16   AXP2101 PWRON key, mirrored to GPIO16 (active high)
  ====== ======== ====================================================

The PWR key is wired to the PWRON pin of the AXP2101.  A MOSFET mirrors
its level to GPIO16, so the application can see it as a normal key, but
the AXP2101 still acts on it: a long press powers the board off and a
short press powers it back on.

I2C Bus
=======

All of the on-board peripherals except the display and the SD card share
one I2C bus, I2C0, on GPIO15 (SDA) and GPIO14 (SCL):

  ======== ======== ===============================
  Address  Device   Function
  ======== ======== ===============================
  0x18     ES8311   Audio codec (playback)
  0x34     AXP2101  Power management
  0x40     ES7210   Audio ADC (microphones)
  0x51     PCF85063 Real time clock
  0x5A     CST9220  Touch controller
  0x6B     QMI8658  IMU
  ======== ======== ===============================

Pin Mapping
===========

  ========== ==================================================
  GPIO       Function
  ========== ==================================================
  GPIO0      BOOT key
  GPIO1      microSD MOSI
  GPIO2      microSD SCLK
  GPIO3      microSD MISO
  GPIO4-7    AMOLED QSPI data 0-3
  GPIO8      I2S data out (to ES8311)
  GPIO9      I2S bit clock
  GPIO10     I2S data in (from ES7210)
  GPIO11     Touch interrupt
  GPIO12     AMOLED QSPI chip select
  GPIO13     RTC interrupt
  GPIO14     I2C SCL
  GPIO15     I2C SDA
  GPIO16     PWR key (SYS_OUT)
  GPIO17     IMU interrupt 1
  GPIO18     IO18 key
  GPIO19/20  USB D-/D+
  GPIO21     IMU interrupt 2
  GPIO38     AMOLED QSPI clock
  GPIO39     AMOLED reset
  GPIO40     Touch reset
  GPIO41     microSD chip select
  GPIO42     I2S master clock
  GPIO43/44  UART0 TX/RX
  GPIO45     I2S word select
  GPIO46     Speaker amplifier enable
  ========== ==================================================

Configurations
==============

All of the configurations presented below can be tested by running the
following commands::

    $ ./tools/configure.sh esp32s3-touch-amoled-216:<config_name>
    $ make flash ESPTOOL_PORT=/dev/ttyACM0 -j

Where ``<config_name>`` is the name of the board configuration you want to
use, e.g. ``nsh``.  To access the console, open ``/dev/ttyACM0`` with a
serial terminal such as ``picocom`` or ``minicom``.

audio
-----

This configuration registers the audio devices.  Both codecs share I2S0
(MCLK GPIO42, BCLK GPIO9, WS GPIO45) in full-duplex mode:

  ====================== ======== ===================================
  Device                 Codec    Function
  ====================== ======== ===================================
  ``/dev/audio/pcm0``    ES8311   Playback, I2S data out on GPIO8
  ``/dev/audio/pcm_in0`` ES7210   Microphones, I2S data in on GPIO10
  ====================== ======== ===================================

The ES8311 ADC is not wired to anything on this board, so it is not
registered as a capture device.  Both devices share the I2S clocks and
therefore must use the same sample rate.

The speaker power amplifier (GPIO46) is only switched on when
``CONFIG_ESP32S3_TOUCH_AMOLED_216_SPEAKER`` is enabled, which this
configuration does.  The microSD card is enabled to hold audio files.  To
play a WAV file::

    nsh> mount -t vfat /dev/mmcsd0 /mnt
    nsh> nxplayer
    nxplayer> device /dev/audio/pcm0
    nxplayer> play /mnt/ping.wav
    nxplayer> q

and to record from the microphones (2 channels, 16 bit, 16 kHz)::

    nsh> nxrecorder
    nxrecorder> device /dev/audio/pcm_in0
    nxrecorder> recordraw /mnt/rec.pcm 2 16 16000
    nxrecorder> stop
    nxrecorder> q

The nxplayer ``tone`` command sends raw PCM without a WAV header, which the
PCM decoder in front of the ES8311 only accepts with
``CONFIG_AUDIO_FORMAT_RAW``.

buttons
-------

This configuration shows the use of the buttons subsystem.  It can be used
by executing the ``buttons`` application and pressing the BOOT, IO18 or
PWR key::

    nsh> buttons
    buttons_main: Starting the button_daemon
    buttons_main: button_daemon started
    button_daemon: Running
    button_daemon: Opening /dev/buttons
    button_daemon: Supported BUTTONs 0x07
    BOOT was pressed
    BOOT was released
    IO18 was pressed
    IO18 was released
    PWR was pressed
    PWR was released

i2c
---

This configuration enables the I2C bus driver and the ``i2c`` tool, which
can be used to inspect the on-board peripherals::

    nsh> i2c dev 0x08 0x77
         0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    00:                         -- -- -- -- -- -- -- --
    10: -- -- -- -- -- -- -- -- 18 -- -- -- -- -- -- --
    20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    30: -- -- -- -- 34 -- -- -- -- -- -- -- -- -- -- --
    40: 40 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    50: -- 51 -- -- -- -- -- -- -- -- 5a -- -- -- -- --
    60: -- -- -- -- -- -- -- -- -- -- -- 6b -- -- -- --
    70: -- -- -- -- -- -- -- --

lcd
---

This configuration drives the 480x480 AMOLED panel (CO5300 controller) on
SPI2 in quad mode (QSPI) and exposes it as a framebuffer at ``/dev/fb0``.
The framebuffer and the driver shadow buffer (used to widen updates to the
even-aligned areas the panel requires) are allocated from PSRAM, so this
configuration builds on ``psram``.  The ``fb`` example draws a series of
nested rectangles::

    nsh> fb
    VideoInfo:
          fmt: 11
         xres: 480
         yres: 480
      nplanes: 1
    PlaneInfo (plane 0):
        fbmem: 0x3c0c0818
        fblen: 460800
       stride: 960
      display: 0
          bpp: 16
    Mapped FB mem: 0x3c0c0818
     0: (  0,  0) (480,480)
     1: ( 43, 43) (394,394)
     2: ( 86, 86) (308,308)
     3: (129,129) (222,222)
     4: (172,172) (136,136)
     5: (215,215) ( 50, 50)
    Test finished

Note that in this configuration SPI2 and SPI3 use the quad I/O mode of
the SPI driver, so the microSD card (``sdmmc``) cannot be used at the same
time.

lvgl
----

This configuration runs the LVGL widgets demo on the AMOLED panel with
touch input (CST9220).  LVGL writes directly to the LCD character device
(``/dev/lcd0``) in partial mode, with a 40-line draw buffer; its rounder
honours the even-aligned areas the panel requires, so no shadow buffer is
needed::

    nsh> lvgldemo

The configuration is tuned for drawing speed: ``-O2`` instead of ``-Os``,
flash in QIO mode at 80 MHz,
PSRAM at 80 MHz, a 64 KiB data cache with 64-byte lines, 32 KiB QSPI
transfers sent by the CO5300 transmit thread
(``CONFIG_LCD_CO5300_TXTHREAD``) and a 1 ms system tick for LVGL's timer.
With ``CONFIG_LV_USE_DEMO_BENCHMARK`` the ``lvgldemo benchmark`` command
reports about 20 FPS on average over all scenes; most scenes run at the
LVGL refresh limit of 30 FPS and are then limited by rendering, not by the
display bus.

Fast full-screen animations may show tearing: the panel's tearing effect
(TE) output only goes to a test pad and is not connected to the ESP32-S3,
so updates cannot be synchronized with the panel refresh.  Sending a full
frame also takes longer (about 23 ms at 40 MHz) than one panel refresh.

nsh
---

Basic NuttShell configuration (console enabled on the USB Serial/JTAG
controller).

qmi8658
-------

This configuration registers the QMI8658 6-axis IMU through the uORB
sensor framework, as ``/dev/uorb/sensor_accel0`` and
``/dev/uorb/sensor_gyro0``.  The ``uorb_listener`` application can be used
to read it.  Acceleration is reported in m/s^2, so with the board lying
flat the Z axis reads about 9.8::

    nsh> uorb_listener -n 3 sensor_accel0

    Monitor objects num:1
    object_name:sensor_accel, object_instance:0
    sensor_accel(now:6660000):timestamp:6660000,x:0.221464,y:0.216675,z:9.764751,temperature:35.015625
    sensor_accel(now:7670000):timestamp:7670000,x:0.228646,y:0.191536,z:9.779117,temperature:35.007812
    sensor_accel(now:8680000):timestamp:8680000,x:0.178368,y:0.135272,z:9.941922,temperature:35.015625
    Object name:sensor_accel0, received:3
    Total number of received Message:3/3

power
-----

This configuration registers the AXP2101 PMIC as a battery charger at
``/dev/batt0`` and its fuel gauge at ``/dev/gauge0``, and enables the NSH
``poweroff`` command, which switches the board off through the AXP2101
(the PWR key turns it back on).  The power rails are left as the AXP2101
OTP configures them.  The power-off voltage, below which the AXP2101 turns
the board off to protect the LiPo battery from deep discharge, is raised
from the 2.6 V of the OTP to 3.0 V (``CONFIG_AXP2101_VOFF_VOLTAGE``).  The
battery is charged to 4.2 V with 200 mA, as programmed in the OTP.
``batterydump`` shows the charger and gauge state::

    nsh> batterydump -t 0 /dev/batt0
    mask:0, state:3, online:1, health:1, vol:4097, protocol:0

    nsh> batterydump -t 1 /dev/gauge0
    BATIOC_CURRENT failed -1
    mask:0, state:3, online:1, vol:4161mV, capacity:100%, current:0mA, temperature:38.7C

Here the battery is full (state 3), USB power is present (online 1) and
the battery is healthy.  The AXP2101 has no current sense, so the gauge
does not report a current: ``batterydump`` prints that the
``BATIOC_CURRENT`` request failed, which is expected.  The temperature is
that of the AXP2101 die.  The raw registers can be
inspected with the ``i2c`` tool, e.g. the chip ID::

    nsh> i2c get -a 34 -r 3
    READ Bus: 0 Addr: 34 Subaddr: 03 Value: 4a

psram
-----

This configuration enables the 8MB octal PSRAM and adds it to the heap.
The ``free`` command shows the extra memory, and the ``heap`` test can be
used to exercise it::

    nsh> free
          total       used       free    maxused    maxfree  nused  nfree name
        8775692      23020    8752672      23400    8388592     75      4 Umem

The PSRAM runs at 80 MHz with a 64 KiB data cache (64-byte lines), which
copies data in PSRAM about 2.6 times as fast as the defaults (40 MHz,
32 KiB).  The ``lcd``, ``lvgl`` and ``audio`` configurations use the same
settings.  All configurations read the flash in QIO mode at 80 MHz.

rtc
---

This configuration uses the on-board PCF85063, which has its own backup
supply, as the system real time clock.  The system time is loaded from it
at boot and ``date -s`` writes it back::

    nsh> date -s "Nov 25 16:55:35 2026"
    nsh> date
    Wed, Nov 25 16:55:36 2026

After a reset, the time is still correct::

    nsh> date
    Wed, Nov 25 16:55:47 2026

The PCF85063 provides the system RTC interface itself, so this
configuration cannot be combined with ``CONFIG_RTC_DRIVER``, which
registers the ESP32-S3 internal RTC instead.

sdmmc
-----

This configuration binds the microSD card slot, on SPI3 (GPIO2 SCLK, GPIO1
MOSI, GPIO3 MISO, GPIO41 CS), to ``/dev/mmcsd0`` and enables the FAT file
system.  The slot has no card detect switch, so the card must be inserted
before the board boots::

    nsh> mount -t vfat /dev/mmcsd0 /mnt
    nsh> echo hello-from-nuttx > /mnt/nxtest.txt
    nsh> cat /mnt/nxtest.txt
    hello-from-nuttx
    nsh> dd if=/dev/zero of=/mnt/big.bin bs=4096 count=256
    1048576 bytes (256 blocks) copied, 800000 usec, 1280 KB/s
    nsh> df -h
      Filesystem      Size      Used  Available Mounted on
      vfat           3580M     1040K      3579M /mnt

touch
-----

This configuration registers the CST9220 capacitive touch controller
(I2C0 address 0x5A, interrupt on GPIO11, reset on GPIO40) as
``/dev/input0`` and includes the ``tc`` touchscreen example.  At boot the
driver identifies the controller::

    cst92xx_probe: CST9220: checkcode 0xcaca4e20 fw 0x01000006 resolution 480x480
    cst92xx_register: Registered /dev/input0, 480x480, 1 points

Run ``tc`` and touch the screen to see the reported coordinates.

wifi
----

Enables Wi-Fi support (station mode) with the ``wapi`` tool.  You can scan
for networks and connect to an access point with::

    nsh> wapi scan wlan0
    nsh> wapi psk wlan0 mypasswd 3
    nsh> wapi essid wlan0 myssid 1
    nsh> renew wlan0

``wapi save_config wlan0`` stores the current settings on the SPI flash,
where ``CONFIG_WIRELESS_WAPI_INITCONF`` picks them up at boot.
