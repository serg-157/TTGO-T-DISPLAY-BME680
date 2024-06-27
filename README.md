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

## Comments

- IDE: VS Code [PlatformIO](https://platformio.org/), project config file: [platformoi.ini](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/platformio.ini)

- The sensor manufacturer's BSEC library was used instead of the simpler and more popular [Adafruit_BME680](https://github.com/adafruit/Adafruit_BME680) library to access additional available parameters

- To fully utilise the ESP32's dual core CPU, [FreeRTOS multitasking](https://www.freertos.org/implementation/a00004.html) is used to process the button press and update the screen in a separate task, while the main loop is used to read the sensor data.
  
  ```ino
  void drawTheScreen(void *arg)
  {
    while (1)
    {
      processButtonPress();
      drawStatusLine();
      drawBatteryIndicator();
      drawMeters();
      vTaskDelay(100);
    }
  }
  
  xTaskCreate(drawTheScreen, "drawTheScreen", 8192, NULL, tskIDLE_PRIORITY, NULL);
  ```

## Building and flashing

- Install VS Code and the [PlatformIO](https://platformio.org/) plugin
- Create new project and select **ttgo-lora32-v1** board
- Add dependencies:
  - [TFT_eSPI Display Library](https://github.com/Bodmer/TFT_eSPI)
  - [BME680 BSEC Library](https://github.com/BoschSensortec/BSEC-Arduino-library)
  - [Battery 18650 Stats Library](https://github.com/danilopinotti/Battery18650Stats)
- Set the correct target board for TFT_eSPI library by editing the contents of pio/libdeps/esp32dev/TFT_eSPI/User_Setup_Select.h file:
  - comment out the line `#include <User_Setup.h>`
  
  - uncomment the line `#include <User_Setups/Setup25_TTGO_T_Display.h>` 
- Replace the contents of the default main.cpp file with [main.cpp](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/src/main.cpp) from this repository
- Enable the console output if required `#define CONSOLE_OUTPUT true`
- Connect the TTGO T-Display board with battery and sensor to your computer using the USB-C data cable.
- Compile the project and upload it to the board



## Pinouts and wiring

![Wiring](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/schematics.jpg)

## Modelling

![Modelling](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/modelling.jpg)

## Unit body and layout

The unit is housed in a transparent case with power and screen control buttons. The sensor is located in a separate compartment as far away from the ESP32 controller as possible and is separated by foamed plastic to minimize the effect on temperature readings.

<img src="https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/front.jpg" width="400"/><img src="https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/back.jpg" width="400"/>

## UI/UX

The sensor readings are displayed on five separate pages with the button toggling between them. At the top of the screen is a status bar that displays the sensor accuracy indicator, air quality and battery level.

### Page 1: Temperarure and humidity

Ring meters showing temperature in degrees Celsius and relative humidity:

<img src="https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen1.jpg" width="280"/>

### Page 2: Air quality

Ring meters showing CO2 and breath VOC equivalents in ppm:

<img src="https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen2.jpg" width="280"/>

### Page 3: Atmospheric pressure history

Graph showing the atmospheric pressure trend over the last few hours and the current pressure in mm of mercury:

<img src="https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen3.jpg" width="280"/>

### Page 4: Dew point

Ring meter showing dew point in degrees Celsius

<img src="https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen4.jpg" width="280"/>

### Page 5: Battery status

Ring meters showing board voltage and percentage of battery charge remaining:

<img src="https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/screen5.jpg" width="280"/>

## Video: alcohol vapour detection

[![Alcohol-detection](https://github.com/serg-157/TTGO-T-DISPLAY-BME680/blob/main/media/alcohol.jpg)](https://player.vimeo.com/video/965910353)