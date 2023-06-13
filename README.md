![Logo](/readme_images/logo_sm.jpg)
# PicoHAL GRBLHAL Plugin

<img src="/readme_images/Board_Photo.jpg" width="800">

This plugin adds support for connecting the PicoHAL board over Modbus to GRBLHAL.  Preliminary code, both hardware and firmware are work-in-progress.

#### The PicoHAL has the following characteristics:
  - Familiar Arduino Uno inspired form factor
  - RS485 input and pass-through
  - 5-24V power input (can be USB powered)
  - Works with CNC shield
  - Works with Relay shield
  - Works with stackable relay shields

#### Shield I/O:
  - 9 true 5V outputs
  - 9 true 5V inputs
  - 5V UART interface
  - 5V I2C interface
  - 2 5V tolerant analog inputs

In addition, there are plug headers for the following:
  - 4 2 amp relay drivers (Labelled Red/Green/Blue/White
  - Buffered 5V Neopixel driver
  - 5V tolerant user input
  
Finally it has I2C exposed to easily mount a small 1.3 inch OLED for more visual feedback.
