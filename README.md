

# Overview
wifi-smartmeter is device a smartmeter measures your current power consumption and post it to prometheus/grafana instance. 

It counts the number of blinks in your current not-so smartmeter. 

Universally, those electronic meters has an led on it and every blink corresponds to a defined amount of energy passing through the meter (kWh/Wh). For single-phase domestic electricity meters each pulse usually equals one Wh (or 1000 pulses per kWh).

## Features
- Costs $8 to build
- With a re-chargable battery lasts months
- Non-invasive measurement, you don't need to connect to your powerline
- Easy to build DIY project with few soldering required.
- Easy to re-purpose for other IoT projects


## Screenhots and Photos
<img src="https://raw.githubusercontent.com/devopswise/wifi-smartmeter/master/image/wifi_smartmeter_bb.png" width="300">
<img src="https://raw.githubusercontent.com/devopswise/wifi-smartmeter/master/image/inside_housing.jpg" width="300">
<img src="https://raw.githubusercontent.com/devopswise/wifi-smartmeter/master/image/grafana1.png" height="300">

some more here: https://imgur.com/a/BOavfxk

## Bill of Materials

1. digispark attiny85
2. esp8266 esp-01 board
3. ams1117 3.3v regulator module
4. lm393 light sensor module
5. powerbank case & 18630 battery
6. dupont wires


Other supplements and components (for development)
- breadboard
- buttons (dev only)
- leds (dev only)
- arduino nano for debugging (dev only)
- usb powermeter
- heatshrink (for better isolation)
- two sided sticker (to mount to it's place next to your not-so smart meter)


# Background
I was interested putting power usage of home on grafana. However there isnt any device provided by my energy company. Even if they have provided I wasnt sure it is not going to have an API.

So this project came to my mind. 
However there is no API provided by my energy company, even no smart meter provided by them.

On the other hand, physically attaching devices to powerline is dangerous. There are many safety precautions needs to be taken. It doesnt worth that risk.

Then I notice there is a LED flashing on our powermeter. I notice it blinks for every w/h energy. So I decided to make a device monitor this led.

I came with an this arduino + esp8266 solution to count  number of blinks. Then I have ported my program to attiny85.

When searching a case for my project, I notice this £1 powerbanks may be good option. I bought one and replaced its 18650 battery with shorter version which is 18630.

## Development
It is difficult to develop code directly on digispark attiny85, as its flash and sram is very limited. 
Same code compiles for both ATMega328P and ATtiny85. It is easier to develop on Arduino Nano then compile for Attiny85.

## Feature Requests
I really would like to hear, but I can't guarantee its implementation, as this is just a hobby project. Forks are always welcome.

## Contributions
Any contributions & questions are welcome.

## Credits

* Deep sleep implementation based on [Ron's 3 Minute Game Timer](https://www.instructables.com/id/3-Minute-Game-Timer/)

* Most of the arduino code based on [ESPTempLogger](https://github.com/guibom/ESPTempLogger)

* More to come...

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

