# TTGO T-Display ESP32 & BME680: portable weather station and air quaility monitor

## Concept

A portable, battery-powered air quality monitor that doesn't require network access and shows real-time data on a built-in display.

Project developed in arduino, the documentation will not be very extensive, but it will be enough to understand the operation of the code, if you have basic knowledge of the technologies used it should be very easy to understand.

## Hardware

- [ESP32](https://www.espressif.com/en/products/hardware/esp32/overview)
- [BM680](https://www.adafruit.com/product/3660)
- [OLed Display](https://www.amazon.com.mx/DIYmall-pulgadas-Display-SSD1306-Raspberry/dp/B09S3SD6KT)

## Libraries

- [Adafruit BME680 Library](https://github.com/adafruit/Adafruit_BME680)
- [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit Sensor Library](https://github.com/adafruit/Adafruit_Sensor)

## References

- [BME680 with ESP32 using Arduino IDE (Gas, Pressure, Temperature, Humidity)](https://microcontrollerslab.com/bme680-esp32-arduino-oled-display/) Please use this as reference for the pinouts.

## Credentials and keys

Remember to change the constants in the `BME680-OLED_PocketBase.ino` file with your credentials and keys.

Wifi credentials:

```ino
const char* ssid = "SSID"; // Network SSID
const char* password = "password"; // Network password
```

PocketBase credentials:

```ino
http.begin("https://custom.domain/api/collections/collection_name/records"); // Specify the URL
```