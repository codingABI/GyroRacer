# GyroRacer
Simple arcade like motorcycling racing game controlled by gyroscope sensor. Game is short and more a technical demo.

![screenshot](/assets/images/Screenshot.png) 

![capture](/assets/images/Capture.gif) 

Online simulation on [Wokwi](https://wokwi.com/projects/332016926716854868)

Watch version 0.1.0 on [Youtube](https://youtu.be/_jVGUhDsQbE)

## License and copyright
My code is licensed under the terms of the MIT License [Copyright (c) 2022-2026 codingABI](LICENSE).

## Hardware
Arduino Uno/Nano with gyroscope sensor MPU6050, SSD1306 OLED 128x64 pixel display and an optional passive buzzer.

![breadboard](/assets/images/Breadboard.svg) 

Running on breadboard was not very stable for me over time (occasionally i2c-bus freezes). The same circuit soldered on board ran stable.

# Appendix

## Schematic

![schema](/assets/images/Schema.svg) 

The interrupt line between MPU6050 pin "INT" and Arduino pin "D2" is currently not used in my code and the device should work without connecting these pins.

## Used development environment

- Arduino IDE 2.3.4 or 1.8.19
- Arduino AVR Boards Version 1.8.6
- Adafruit GFX Library 1.11.11 
- Adafruit SSD1306 2.5.13 (dont forget to uncomment #define SSD1306_NO_SPLASH in Adafruit_SSD1306.h to prevent the "Sketch too big" error)
- I2Cdev (from https://github.com/jrowberg/i2cdevlib)

## Player sprite

![Player sprite](/assets/images/PlayerSprite.png)

![Animated player sprite](/assets/images/PlayerSprite.gif) 
