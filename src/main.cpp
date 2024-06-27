/////////////////////////////////////////////////////////////////
/*
  Portable weather station and air quaility monitor
  TTGO T-Display ESP32 & BME680 environmental sensor
  Created by Sergey Dushkin
*/
/////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <bsec.h>
#include <Battery18650Stats.h>

// Console output flag
#define CONSOLE_OUTPUT false

// TFT display library Initializing
TFT_eSPI tft = TFT_eSPI();

// "Next Page" button pin
#define BUTTON_PIN 25       // GPIO pin connected to button
int buttonLastState = HIGH; // The previous state from the input pin
int pageNumber = 1;
int drawnPageNumber = 0;

// BME680 sensor
Bsec iaq;
bool newDataReceived = false;
String sensorStatus = "";
#define TEMPERATURE_COMPENSATION -1.3
String statusLastPrinted = "";
uint8_t accuracyLastSeen = -1;

// Pressure measurement history
#define PRESSURE_POINTS 240
float pressure[PRESSURE_POINTS];
int pressurePrinted[2][PRESSURE_POINTS];
bool pressureCurveRefreshNeeded = false;
float pressureMin = 0;
float pressureMax = 0;
float pressureLastSeen = 0;
String pressureLabelLastSeen = "";
String pressureLabelMinLastSeen = "";
String pressureLabelMaxLastSeen = "";
int pressureValues = 0;
int pressureValuesPrinted = 0;

// Battery icon size and color
#define ICON_WIDTH 36
#define ICON_HEIGHT 17
#define STATUS_BAR_HEIGHT ICON_HEIGHT
#define ICON_POS_X (tft.width() - ICON_WIDTH)

// Custom colors
#define TFT_DARK_GRAY 0x18E3

// Battery pin and voltage params
#define ADC_PIN 34
#define MIN_USB_VOL 4.72
#define CONVERSION_FACTOR 1.8
#define READS 20
int batteryLevel = 0;
int drawnBatteryLevel = -1;

// Ring meter color schemes
#define RED2RED 0
#define GREEN2GREEN 1
#define BLUE2BLUE 2
#define BLUE2RED 3
#define GREEN2RED 4
#define RED2GREEN 5

// Ring meter parameters
#define RING_RADIUS 59
#define RING_GAP 3
#define RING_AREA_START_Y STATUS_BAR_HEIGHT + 12
#define ACCURACY_BOX_HEIGHT RING_AREA_START_Y - 8
#define ACCURACY_BOX_WIDTH 13

// Battery library initialization
Battery18650Stats battery(ADC_PIN, CONVERSION_FACTOR, READS);



// >>> Pinouts
void initPinout()
{
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
  // "Next Page" button
  pinMode(BUTTON_PIN, INPUT_PULLUP); // config GPIO as input pin and enable the internal pull-up resistor
}

// >>> Display initialization
void initDisplay()
{
  tft.begin();
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  tft.setTextFont(4);
  
  drawnPageNumber = 0;
  drawnBatteryLevel = -1;
  statusLastPrinted = "";

  if (CONSOLE_OUTPUT) Serial.println("Display initialized");
}

// >>> Process "Next page" button press
void processButtonPress()
{
  // Read the state of the switch/button
  int buttonCurrentState = digitalRead(BUTTON_PIN);
  
  if (buttonCurrentState == HIGH && buttonLastState == LOW)
  {
    pageNumber++;
    if (pageNumber == 6) pageNumber = 1;
  }
  
  buttonLastState = buttonCurrentState;
}

// >>> BME680 sensor initialization
void initIaqSensor(void)
{
  delay(1000);

  iaq.begin(BME68X_I2C_ADDR_HIGH, Wire);

  bsec_virtual_sensor_t sensorList[13] = {
      BSEC_OUTPUT_IAQ,
      BSEC_OUTPUT_STATIC_IAQ,
      BSEC_OUTPUT_CO2_EQUIVALENT,
      BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
      BSEC_OUTPUT_RAW_TEMPERATURE,
      BSEC_OUTPUT_RAW_PRESSURE,
      BSEC_OUTPUT_RAW_HUMIDITY,
      BSEC_OUTPUT_RAW_GAS,
      BSEC_OUTPUT_STABILIZATION_STATUS,
      BSEC_OUTPUT_RUN_IN_STATUS,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
      BSEC_OUTPUT_GAS_PERCENTAGE};

  iaq.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
}

// >>> Print out sensor error message
void drawSensorErrorScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("Sensor error -", 20, 20, 4);
  tft.drawString("check wiring!", 20, 45, 4);
}

// >>> BME680 sensor status check
void checkIaqSensorStatus(void)
{
  if (iaq.bsecStatus == BSEC_OK && iaq.bme68xStatus == BME68X_OK)
  {
    sensorStatus = "OK";
    return;
  }

  if (iaq.bsecStatus != BSEC_OK)
  {
    if (iaq.bsecStatus < BSEC_OK)
    {
      for (int i = 1; i <= 10; i++) // trying to reinitialize the sensor
      {
        initIaqSensor();
        if (iaq.bsecStatus >= BSEC_OK) break;
        delay(200);
      }
      if (iaq.bsecStatus < BSEC_OK)
      {
        sensorStatus = "BSEC error " + String(iaq.bsecStatus);
        for (;;) // Print out error message and halt
        {
          drawSensorErrorScreen();
          delay(400);
        }
      }
    }
    else
    {
      sensorStatus = "BSEC warng " + String(iaq.bsecStatus);
    }
  }

  if (iaq.bme68xStatus != BME68X_OK)
  {
    if (iaq.bme68xStatus < BME68X_OK)
    {
      for (int i = 1; i <= 10; i++) // trying to reinitialize the sensor
      {
        initIaqSensor();
        if (iaq.bsecStatus >= BME68X_OK) break;
        delay(200);
      }
      if (iaq.bme68xStatus < BME68X_OK)
      {
        sensorStatus = "BME680 error " + String(iaq.bme68xStatus);
        for (;;) // Print out error message and halt
        {
          drawSensorErrorScreen();
          delay(400);
        }
      }
    }
    else
    {
      sensorStatus = "BME680 warng " + String(iaq.bme68xStatus);
    }
  }
}

// >>> BME680 sensor data printout
void printSensorData(unsigned long timeStamp)
{
  String output;

  output = "Timestamp [ms], IAQ, IAQ accuracy, Static IAQ, CO2 equivalent, breath VOC equivalent, raw temp[°C], pressure [hPa], raw relative humidity [%], gas [Ohm], Stab Status, run in status, comp temp[°C], comp humidity [%], gas percentage";
  Serial.println(output);

  output = String(timeStamp);
  output += ", " + String(iaq.iaq);
  output += ", " + String(iaq.iaqAccuracy);
  output += ", " + String(iaq.staticIaq);
  output += ", " + String(iaq.co2Equivalent);
  output += ", " + String(iaq.breathVocEquivalent);
  output += ", " + String(iaq.rawTemperature);
  output += ", " + String(iaq.pressure);
  output += ", " + String(iaq.rawHumidity);
  output += ", " + String(iaq.gasResistance);
  output += ", " + String(iaq.stabStatus);
  output += ", " + String(iaq.runInStatus);
  output += ", " + String(iaq.temperature);
  output += ", " + String(iaq.humidity);
  output += ", " + String(iaq.gasPercentage);
  Serial.println(output);
}

// >>> Init the pressure array
void initPressureArrays(void)
{
  for (int i = 0; i < PRESSURE_POINTS; i++)
    pressure[i] = 0;

  for (int i = 0; i < 2; i++)
    for (int j = 0; j < PRESSURE_POINTS; j++)
      pressurePrinted[i][j] = 0;
}

// >>> Add to the pressure array
void addPressureValue(float value)
{
  int emptySlot = -1;
  float min = pressure[0], max = pressure[0];
  int count = 0;
  for (int i = 0; i < PRESSURE_POINTS; i++)
  {
    if ((int)pressure[i] == 0)
    {
      emptySlot = i;
      break;
    }
    if (pressure[i] < min)
      min = pressure[i];
    if (pressure[i] > max)
      max = pressure[i];
    count++;
  }
  if (emptySlot == -1) // no slots left, shifting
  {
    for (int i = 0; i < PRESSURE_POINTS - 1; i++)
    {
      pressure[i] = pressure[i + 1];
    }
    emptySlot = PRESSURE_POINTS - 1;
  }
  // Save the measure
  pressure[emptySlot] = value;
  // Save max/min
  pressureMin = min;
  pressureMax = max;
  pressureValues = count;
  pressureCurveRefreshNeeded = true;
}

// >>> Return a 16 bit rainbow color
unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0;   // Red is the top 5 bits of a 16 bit colour value
  byte green = 0; // Green is the middle 6 bits
  byte blue = 0;  // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0)
  {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1)
  {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2)
  {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3)
  {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}


// >>> Draw the meter on the screen, returns x coord of righthand side
int ringMeter(float data, int vmin, int vmax, int x, int y, int r, const char *units, byte scheme, unsigned int decimalPlaces, int ringAngle)
{
  int value = data;

  if (data > 999) decimalPlaces = 0;

  int multiplier = 1;
  if (decimalPlaces > 0)
  {
    multiplier = pow(10, decimalPlaces);
    value = data * multiplier;
    vmin = vmin * multiplier;
    vmax = vmax * multiplier;
  }

  // Center of the ring: minimum value of r is about 52 before value text intrudes on ring, drawing the text first is an option
  x += r;
  y += r;
  int w = r / 3; // Width of outer ring is 1/3 of radius

  int angle = ringAngle / 2; // Half the sweep angle of meter (300 degrees)

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 3; // Segments are 3 degrees wide = 100 segments for 300 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Variable to save "value" text colour from scheme and set default
  int color = TFT_BLUE;
  
  // Draw colour blocks every inc degrees
  for (int i = -angle + inc / 2; i < angle - inc / 2; i += inc)
  {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v)
    { // Fill in colored segments with 2 triangles
      switch (scheme)
      {
      case 0:
        color = TFT_RED;
        break; // Fixed color
      case 1:
        color = TFT_GREEN;
        break; // Fixed color
      case 2:
        color = TFT_BLUE;
        break; // Fixed color
      case 3:
        color = rainbow(map(i, -angle, angle, 0, 127));
        break; // Full spectrum blue to red
      case 4:
        color = rainbow(map(i, -angle, angle, 70, 127));
        break; // Green to red (high temperature etc)
      case 5:
        color = rainbow(map(i, -angle, angle, 127, 63));
        break; // Red to green (low battery etc)
      default:
        color = TFT_BLUE;
        break; // Fixed color
      }
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, color);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_DARK_GRAY);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_DARK_GRAY);
    }
  }

  // Convert value to a string
  String out = String(data, decimalPlaces);

  // Set the text color to default
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  uint8_t font = (r > 84) ? 8 : 4; // Font size depending on ring radius
  uint16_t padding = (r > 84) ? 5 * 55 : 5 * 14; // Allow for 5 digits of corresponding character width

  tft.setTextPadding(padding); // Set padding
  tft.drawString(out, x, y, font); // Print value in the middle

  // Print units, if the meter is large then use big font 4, othewise use 2
  font = (r > 84) ? 4 : 2; // Font size for units
  int32_t shift = (r > 84) ? 60 : 15;
  tft.drawString(units, x, y + shift, font); // Print units
  
  // Calculate and return right hand side next X coordinate
  return x + r;
}

// >>> Draw pressure curve
void drawPressureGraph(float value)
{
  int dotsX = 17, dotsY = 7, gridStepX = 15, gridStepY = 17;
  int xCoord = 0, yCoord = STATUS_BAR_HEIGHT + 15, xSize = tft.width(), ySize = (dotsY - 1) * gridStepY;

  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);

  // Erasing a previously printed curve to avoid having to clean the entire graph area and reduce flickering
  if (pressureValuesPrinted > 0 && pressureCurveRefreshNeeded)
  {
    for (int i = 0; i < pressureValues; i++)
      tft.drawPixel(pressurePrinted[0][i], pressurePrinted[1][i], TFT_BLACK); // fit in the plot area
  }  

  // Print the current pressure value
  String pressureLabel = String(value, 1);
  int labelShift = 176;
  tft.setTextPadding(5 * 14);
  if (!pressureLabel.equals(pressureLabelLastSeen))
  {
    tft.setTextColor(TFT_BLACK);
    tft.drawString(pressureLabelLastSeen, xCoord + labelShift, yCoord, 4);  // Clear label area
    pressureLabelLastSeen = pressureLabel;
  }
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(pressureLabel, xCoord + labelShift, yCoord, 4);

  // Draw grid dots
  int x = xCoord, y = yCoord;
  for (int k = 1; k <= dotsX; k++) // X axis dots
  {
    for (int n = 1; n <= dotsY; n++) // Y axis dots
    {
      tft.drawPixel(x - 1, y, TFT_YELLOW);
      y = y + gridStepY;
    }
    y = yCoord;
    x = x + gridStepX;
  }

  // Draw X and Y axis
  tft.setTextColor(TFT_WHITE);
  tft.drawLine(xCoord, yCoord + ySize, xSize, yCoord + ySize, TFT_WHITE);  // X axis
  tft.drawLine(xCoord, yCoord, xCoord, yCoord + ySize, TFT_WHITE); // Y axis

  if (pressureValues == 0 || !pressureCurveRefreshNeeded) return; // nothing to draw

  float pressureAvg = (pressureMin + pressureMax) / 2.0;
  float pressureDif = pressureMax - pressureMin;
  pressureDif = ((pressureDif < 1.0) ? 1.0 : pressureDif) / 2;
  float yValueMin = pressureAvg - pressureDif;
  float yValueMax = pressureAvg + pressureDif;
  double scale = ySize / (yValueMax - yValueMin);

  // Draw Y axis lables
  String labelMin = String(yValueMin, 1);
  String labelMax = String(yValueMax, 1);
  int labelXShift = 4;
  int labelYShift = 17;
  tft.setTextPadding(5 * 7);
  if (!labelMin.equals(pressureLabelMinLastSeen))
  {
    tft.setTextColor(TFT_BLACK);
    tft.drawString(pressureLabelMinLastSeen, xCoord + labelXShift, yCoord + ySize - labelYShift, 2); // erasing previous
    pressureLabelMinLastSeen = labelMin;
  }
  if (!labelMax.equals(pressureLabelMaxLastSeen))
  {
    tft.setTextColor(TFT_BLACK);
    tft.drawString(pressureLabelMaxLastSeen, xCoord + labelXShift, yCoord, 2); // erasing previous
    pressureLabelMaxLastSeen = labelMax;
  }
  tft.setTextColor(TFT_WHITE);
  tft.drawString(labelMin, xCoord + labelXShift, yCoord + ySize - labelYShift, 2);
  tft.drawString(labelMax, xCoord + labelXShift, yCoord, 2);

  // Draw the graph
  x = 1;
  pressureValuesPrinted = 0;
  for (int i = 0; i < pressureValues; i++)
  {
    y = yCoord + (ySize / 2) + (int)round((pressureAvg - pressure[i]) * scale);
    if (y >= yCoord) tft.drawPixel(x, y, TFT_RED); // Print a dot, fitting in the plot area
    pressurePrinted[0][i] = x; // save printed coords to erase the graph before next print
    pressurePrinted[1][i] = y;
    pressureValuesPrinted++;
    x++;
  }

  pressureCurveRefreshNeeded = false;
}

// >>> Dew point calculation
double calculateDewPoint(double temperature, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * temperature) / (b + temperature) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

// >>> Draw accuracy indicator
void drawAccuracyIndicator(int accuracy)
{
  int32_t spacing = 2;
  int32_t blockHeight = ((ACCURACY_BOX_HEIGHT - spacing * 2) / 4) - 2;
  int32_t blockWidth = ACCURACY_BOX_WIDTH - spacing * 2;

  tft.fillRect(0, 0, ACCURACY_BOX_WIDTH, ACCURACY_BOX_HEIGHT, TFT_DARK_GRAY);

  if (accuracy >= 3) // High
    tft.fillRect(spacing, spacing, blockWidth, blockHeight, TFT_SILVER);
  if (accuracy >= 2) // Good
    tft.fillRect(spacing, spacing + (blockHeight + spacing + 1) * 1, blockWidth, blockHeight, TFT_SILVER);
  if (accuracy >= 1) // Low
    tft.fillRect(spacing, spacing + (blockHeight + spacing + 1) * 2, blockWidth, blockHeight, TFT_SILVER);
  if (accuracy >= 0) // Stabilizing
    tft.fillRect(spacing, spacing + (blockHeight + spacing + 1) * 3, blockWidth, blockHeight, TFT_SILVER);
}

// Draw status line
void drawStatusLine()
{
  uint16_t color = TFT_WHITE;
  uint8_t accuracyLatest = iaq.iaqAccuracy;
  float iaqLatest = iaq.iaq;
  
  if (accuracyLastSeen != accuracyLatest)
  {
    drawAccuracyIndicator(accuracyLatest);
    accuracyLastSeen = accuracyLatest;
  }

  String status = "";

  if (accuracyLatest == 0)
  {
    status = "Stabilizing...";
    color = TFT_WHITE;
  }

  if (pageNumber == 3)
  {
    status = "Pressure";
    color = TFT_WHITE;
  }

  if (pageNumber == 5)
  {
    status = "Battery";
    color = TFT_WHITE;
  }

  if (status.isEmpty())
  {
    if ((iaqLatest > 0) && (iaqLatest <= 50))
    {
      status = "Excellent";
      color = TFT_GREEN;
    }
    else if ((iaqLatest > 51) && (iaqLatest <= 100))
    {
      status = "Good";
      color = TFT_GREENYELLOW;
    }
    else if ((iaqLatest > 101) && (iaqLatest <= 150))
    {
      status = "Polluted *"; // Lightly
      color = TFT_YELLOW;
    }
    else if ((iaqLatest > 151) && (iaqLatest <= 200))
    {
      status = "Polluted **"; // Moderately
      color = TFT_ORANGE;
    }
    else if ((iaqLatest > 201) && (iaqLatest <= 250))
    {
      status = "Polluted ***"; // Heavily
      color = TFT_ORANGE;
    }
    else if ((iaqLatest > 251) && (iaqLatest <= 350))
    {
      status = "Polluted ****"; // Severely
      color = TFT_RED;
    }
    else if (iaqLatest > 351)
    {
      status = "Polluted *****"; // Extremely
      color = TFT_MAGENTA;
    }
  }

  if (!status.equals(statusLastPrinted) && !status.isEmpty())
  {
    int textShift = 6;
    tft.setTextSize(1);
    tft.setTextPadding(0);
    tft.fillRect(ACCURACY_BOX_WIDTH + textShift, 0, ICON_POS_X - ACCURACY_BOX_WIDTH - textShift - 1, RING_AREA_START_Y - 1, TFT_BLACK);
    tft.setTextColor(color, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(status, ACCURACY_BOX_WIDTH + textShift, 0, 4);
    statusLastPrinted = status;
  }
}

// >>> Battery info to Serial
void printBatteryInfo()
{
  Serial.print("Volts: ");
  Serial.println(battery.getBatteryVolts());
  Serial.print("Charge level: ");
  Serial.println(battery.getBatteryChargeLevel());
  Serial.print("Charge level (ref): ");
  Serial.println(battery.getBatteryChargeLevel(true));
}

// >>> Draw battery indicator
void drawBatteryIndicator()
{

  if (abs(batteryLevel - drawnBatteryLevel) <= 2) return;

  if (CONSOLE_OUTPUT) printBatteryInfo();

  int chargeLevelCode = 0;
  if (batteryLevel >= 80)
  {
    chargeLevelCode = 3;
  }
  else if (batteryLevel < 80 && batteryLevel >= 50)
  {
    chargeLevelCode = 2;
  }
  else if (batteryLevel < 50 && batteryLevel >= 20)
  {
    chargeLevelCode = 1;
  }
  else if (batteryLevel < 20)
  {
    chargeLevelCode = 0;
  }

  int32_t spacing = 2;
  int32_t edgeSpacing = spacing + 1;
  int32_t blockWidth = ((ICON_WIDTH - edgeSpacing * 2) / 4) - 2;
  int32_t blockHeight = ICON_HEIGHT - edgeSpacing * 2;

  tft.fillRect(ICON_POS_X, 0, ICON_WIDTH, ICON_HEIGHT, TFT_DARK_GRAY);

  switch (chargeLevelCode)
  {
    case 0: // < 20%
      tft.fillRect(ICON_POS_X + edgeSpacing, edgeSpacing, blockWidth, blockHeight, TFT_RED);
      break;
    case 1: // < 50%
      tft.fillRect(ICON_POS_X + edgeSpacing, edgeSpacing, blockWidth, blockHeight, TFT_YELLOW);
      tft.fillRect(ICON_POS_X + edgeSpacing + (blockWidth + spacing + 1) * 1, edgeSpacing, blockWidth, blockHeight, TFT_YELLOW);
      break;
    case 2: // < 80%
      tft.fillRect(ICON_POS_X + edgeSpacing, edgeSpacing, blockWidth, blockHeight, TFT_GREEN);
      tft.fillRect(ICON_POS_X + edgeSpacing + (blockWidth + spacing + 1) * 1, edgeSpacing, blockWidth, blockHeight, TFT_GREEN);
      tft.fillRect(ICON_POS_X + edgeSpacing + (blockWidth + spacing + 1) * 2, edgeSpacing, blockWidth, blockHeight, TFT_GREEN);
      break;
    case 3: // > 80%
      tft.fillRect(ICON_POS_X + edgeSpacing, edgeSpacing, blockWidth, blockHeight, TFT_GREEN);
      tft.fillRect(ICON_POS_X + edgeSpacing + (blockWidth + spacing + 1) * 1, edgeSpacing, blockWidth, blockHeight, TFT_GREEN);
      tft.fillRect(ICON_POS_X + edgeSpacing + (blockWidth + spacing + 1) * 2, edgeSpacing, blockWidth, blockHeight, TFT_GREEN);
      tft.fillRect(ICON_POS_X + edgeSpacing + (blockWidth + spacing + 1) * 3, edgeSpacing, blockWidth, blockHeight, TFT_GREEN);
      break;
    default:
      break;
  }

  drawnBatteryLevel = batteryLevel;
}

// >>> Draw meters
void drawMeters()
{
  if (!newDataReceived && pageNumber == drawnPageNumber) return; // nothing to draw

  // Clear meters area
  if (pageNumber != drawnPageNumber)
  {
    tft.fillRect(0, RING_AREA_START_Y, 240, 135 - RING_AREA_START_Y, TFT_BLACK);
    pressureCurveRefreshNeeded = true;
  }
  
  int xpos;
  int ringRadiusLarge = 82;
  switch (pageNumber)
  {
    case 1: // Temperature (compensated) and Humidity page
      xpos = 0;
      xpos = RING_GAP + ringMeter(iaq.temperature + TEMPERATURE_COMPENSATION, -10, 40, xpos, RING_AREA_START_Y, RING_RADIUS, "Temp *C", BLUE2RED, 1, 300);
      xpos = RING_GAP + ringMeter(iaq.humidity, 15, 90, xpos, RING_AREA_START_Y, RING_RADIUS, "Hum %", RED2GREEN, 1, 300);
      break;
    case 2: // CO2 & VOC page
      xpos = 0;
      xpos = RING_GAP + ringMeter((int)iaq.co2Equivalent, 300, 1400, xpos, RING_AREA_START_Y, RING_RADIUS, "CO2 ppm", GREEN2RED, 0, 300);
      xpos = RING_GAP + ringMeter(iaq.breathVocEquivalent, 0, 1.5, xpos, RING_AREA_START_Y, RING_RADIUS, "VOC ppm", GREEN2RED, 1, 300);
      break;
    case 3: // Pressure page
      drawPressureGraph(pressureLastSeen);
      break;
    case 4: // DewPoint page
      xpos = (tft.width() / 2) - ringRadiusLarge;
      xpos = RING_GAP + ringMeter(calculateDewPoint(iaq.temperature, iaq.humidity), -10, (int)iaq.temperature, xpos, RING_AREA_START_Y + 1, ringRadiusLarge, "Dew Point", GREEN2RED, 1, 194);
      break;
    case 5: // Battery page
      xpos = 0;
      xpos = RING_GAP + ringMeter((float)battery.getBatteryVolts(), 0, 6, xpos, RING_AREA_START_Y, RING_RADIUS, "Volt", BLUE2BLUE, 2, 300);
      xpos = RING_GAP + ringMeter((int)battery.getBatteryChargeLevel(), 0, 100, xpos, RING_AREA_START_Y, RING_RADIUS, "%", RED2GREEN, 0, 300);
      break;
    default:
      break;
  }

  drawnPageNumber = pageNumber;
  newDataReceived = false;
}

// >>>  Screen output task: will be executed as an xTask job
void drawTheScreen(void *arg)
{
  for (;;)
  {
    // Is page change requested?
    processButtonPress();

    // Draw it all!
    drawStatusLine();
    drawBatteryIndicator();
    drawMeters();

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// >>> Setup
void setup()
{
  Serial.begin(9600);

  initPinout();
  initDisplay();
  initPressureArrays();

  // BME680 sensor init
  initIaqSensor();
  checkIaqSensorStatus();

  // Draw screen xTtask
  xTaskCreate(drawTheScreen, "drawTheScreen", 8192, NULL, tskIDLE_PRIORITY, NULL);
}

// >>> Loop
void loop()
{
  unsigned long timeTrigger = millis();

  if (iaq.run()) // New sensor data is available
  {

    sensorStatus = "OK";

    // Read the battery charge level
    batteryLevel = battery.getBatteryChargeLevel();

    // Sensor data console output
    if (CONSOLE_OUTPUT) printSensorData(timeTrigger);

    // Reading and processing pressure
    float currentPressure = iaq.pressure / 100.0 * 0.75006;
    if (fabs(pressureLastSeen - currentPressure) > 0.03)
    {
      addPressureValue(currentPressure);
      pressureLastSeen = currentPressure;
    }

    newDataReceived = true;

  }
  else
  {
    checkIaqSensorStatus();
    if (!sensorStatus.equals("OK"))
    {
      Serial.print("No data, sensor error: ");
      Serial.println(sensorStatus);
    }
  }

  delay(100);
}