# TTGO T-Display ESP32 & BME680: portable weather station and air quaility monitor

## Concept

A portable, battery-powered air quality monitor that doesn't require network access, shows real-time data on a built-in display and can be used anywhere.

## Hardware

- [TTGO T-Display ESP32 development board](https://www.lilygo.cc/products/lilygo%C2%AE-ttgo-t-display-1-14-inch-lcd-esp32-control-board)
- [Gas Sensor BME680 Bosch Sensortec](https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors/bme680/)
- [104050 Li-Po Battery 3.7V 2500mAh](https://www.ebay.com/itm/175539384801)

## Libraries

- [TFT_eSPI Display Library](https://github.com/Bodmer/TFT_eSPI)
- [BME680 BSEC Library](https://github.com/BoschSensortec/BSEC-Arduino-library)
- [Battery 18650 Stats Library](https://github.com/danilopinotti/Battery18650Stats)

## Pinouts and wiring

![Wiring](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/schematics.jpg)

## Unit body and layout

The unit is housed in a transparent case with power and screen control buttons. The sensor is located in a separate compartment as far away from the ESP32 controller as possible and is separated by foamed plastic to minimize the effect on temperature readings.
![Front](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/front.jpg)
![Back](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/back.jpg)

## UI/UX

The sensor readings are displayed on five separate pages with the button toggling between them. At the top of the screen is a status bar that displays the sensor accuracy indicator, air quality and battery level.

### Page 1: Temperarure and humidity
Ring meters showing temperature in degrees Celsius and relative humidity:
![Screen-1](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen1.jpg)

### Page 2: Air quality
Ring meters showing CO2 and breath VOC equivalents in ppm:
![Screen-2](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen2.jpg)

### Page 3: Atmospheric pressure history
Graph showing the atmospheric pressure trend over the last few hours and the current pressure value in millimetres of mercury:
![Screen-3](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen3.jpg)

### Page 4: Dew point
Ring meter showing dew point in degrees Celsius
![Screen-4](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen4.jpg)

### Page 5: Battery status
Ring meters showing board voltage and percentage of battery charge remaining:
![Screen-5](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen5.jpg)


## Detection of alcohol vapour in the air by the sensor
![Alcohol-vapour](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/alcohol_detection.mp4)