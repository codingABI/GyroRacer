# GyroRacer
Simple arcade like motorcycling racing game controlled by gyroscope sensor. Game is short and more a technical demo.

![screenshot](/assets/images/Screenshot.png) 

![capture](/assets/images/Capture.gif) 

Simulation https://wokwi.com/projects/332016926716854868

Video https://youtu.be/_jVGUhDsQbE

## License and copyright
My code is licensed under the terms of the MIT License License [Copyright (c) 2022 codingABI](LICENSE).

## Hardware
Arduino Uno/Nano with gyroscope sensor MPU6050, SSD1306 OLED 128x64 pixel display and an optional passive buzzer.

![breadboard](/assets/images/Breadboard.svg) 

Running on breadboard was not very stable for me over time (occassionally i2c-bus freezes). The same circuit soldered on board ran stable.

# Appendix

## Schematic

![schema](/assets/images/Schema.svg) 

## Development environment

- Arduino IDE 2.3.4 oder 1.8.19
- Adafruit GFX Library 1.11.11 
- Adafruit SSD1306 2.5.13 (dont forget to uncomment #define SSD1306_NO_SPLASH in Adafruit_SSD1306.h to prevent the "Sketch too big" error)
- I2Cdev (from https://github.com/jrowberg/i2cdevlib)

## Player sprite

![Player sprite](/assets/images/PlayerSprite.png)

![Animated player sprite](/assets/images/PlayerSprite.gif) 
