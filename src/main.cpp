
#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <EEPROM.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#define menuCount 4

// --- Constants ---
#define ADJUSTABLE_MENU_ITEMS 4 // Number of items that have a value to adjust
#define TOTAL_MENU_ITEMS 5      // Total number of items including "Save and Exit"

// --- Function Prototypes ---
void handleInputs();
void updateDisplay();
void drawHomeScreen(float distance, const String &status);
void drawMainMenu();
void drawAdjustValue();
float getValue(int index);
// void readUltrasonicCM();
bool oledstate = true;
// --- Global Variables & Objects ---
SSD1306Wire oled(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// Menu state variables
int currentPage = 0;     // 0=Home, 1=Menu, 2=Adjust
int menuIndex = 0;       // Currently highlighted menu item (0-4)
int currentMenu = 0;     // The menu item selected for value adjustment
bool needsRedraw = true; // Flag to redraw the screen only when needed

// Menu items and their corresponding values
String menuItems[TOTAL_MENU_ITEMS] = {"Drought Threshold", "Flood Threshold", "Tank Depth", "Sensor High", "Save and Exit"};
float values[ADJUSTABLE_MENU_ITEMS] = {6.0, 6.0, 6.0, 6.0}; // Note: Size is ADJUSTABLE_MENU_ITEMS

// --- Encoder Pins & Variables ---
int pinA = 36; // CLK
int pinB = 37; // DT
int sw = 38;   // Switch
int pinALast;
int aVal;

//-----------EEPROM SiZE---------------
#define EEPROM_SIZE 8
//-----------EEPROM Address---------------

// Tank height in meters, adjust to your tank
// ---------------- Encoder AND Oled ----------------
// ========== Ultrasonic pins ==========
#define Trig_PIN 46
#define Echo_PIN 45
// Ultrasonic reading function

/* OTAA para*/
uint8_t devEui[] = {0x70, 0xB3, 0xD5, 0x7E, 0xD8, 0x00, 0x4B, 0x7A};
uint8_t appEui[] = {0x00, 0x00, 0x00, 0x83, 0x64, 0x33, 0xE8, 0x64};
uint8_t appKey[] = {0xBC, 0xC5, 0xE2, 0xDB, 0x06, 0x41, 0xAF, 0xC5, 0xDC, 0x48, 0x7A, 0x8B, 0xA0, 0xAC, 0x53, 0x7E};

/* ABP para*/
uint8_t nwkSKey[] = {0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85};
uint8_t appSKey[] = {0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67};
uint32_t devAddr = (uint32_t)0x007e6ae1;

// LoraWan channelsmask, default channels 0-7/
uint16_t userChannelsMask[6] = {0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

// LoraWan region, select in arduino IDE tools/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

// LoraWan Class, Class A and Class C are supported/
DeviceClass_t loraWanClass = CLASS_A;

// the application data transmission duty cycle.  value in [ms]./
uint32_t appTxDutyCycle = 600000;

// OTAA or ABP/
bool overTheAirActivation = true;

// ADR enable/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
 * Number of trials to transmit the frame, if the LoRaMAC layer did not
 * receive an acknowledgment. The MAC performs a datarate adaptation,
 * according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
 * to the following table:
 *
 * Transmission nb | Data Rate
 * ----------------|-----------
 * 1 (first)       | DR
 * 2               | DR
 * 3               | max(DR-1,0)
 * 4               | max(DR-1,0)
 * 5               | max(DR-2,0)
 * 6               | max(DR-2,0)
 * 7               | max(DR-3,0)
 * 8               | max(DR-3,0)
 *
 * Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
 * the datarate, in case the LoRaMAC layer did not receive an acknowledgment
 */

int readUltrasonicCM()
{
  static int lastValid = -1; // remember last good value
  long duration;
  int distanceCm = -1;

  // --- Trigger pulse ---
  digitalWrite(Trig_PIN, LOW);
  delayMicroseconds(3);
  digitalWrite(Trig_PIN, HIGH);
  delayMicroseconds(25);
  digitalWrite(Trig_PIN, LOW);

  // --- Echo pulse duration ---
  duration = pulseIn(Echo_PIN, HIGH); // timeout 30ms (~5m)
  digitalWrite(3, 1);

  if (duration > 0)
  {
    digitalWrite(3, 0);
    distanceCm = duration / 29 / 2; // µs → cm
  }

  // --- Validate distance ---
  if (distanceCm >= 20 && distanceCm <= 600)
  {
    lastValid = distanceCm; // update only if valid
    return distanceCm;
  }
  else
  {
    return lastValid; // return previous good
  }
}

//================Status==========================
String statusss = "normal";
void getstatus(float distance)
{
  if (distance < values[0])
  {
    statusss = "Drought";
    // Turn off LED
  }
  else if (distance > values[1])
  {
    statusss = "Flood";
  }
  else
  {
    statusss = "Normal";
  }
}

//==================EEPROM==========================
void convertEEPROM()
{
  EEPROM.begin(EEPROM_SIZE);
  float droughtThreshold = EEPROM.read(0);
  float floodThreshold = EEPROM.read(2);
  float tankDepth = EEPROM.read(4);
  float sensorHeight = EEPROM.read(6);
  // bool oledState = EEPROM.read(16);

  values[0] = droughtThreshold * 0.01;
  values[1] = floodThreshold * 0.01;
  values[2] = tankDepth * 0.01;
  values[3] = sensorHeight * 0.01;
  // oledstate = oledState;
  EEPROM.end();
}
//===================OLED==========================

void WriteValue()
{
  EEPROM.begin(EEPROM_SIZE);

  // Use EEPROM.put() to correctly store float variables
  EEPROM.write(0, int(values[0] * 100));   // Address 0: Drought Threshold
  EEPROM.write(2, int(values[1] * 100));   // Address 8: Tank Depth
  EEPROM.write(4, int(values[2] * 100)); // Address 12: Sensor Height
  EEPROM.write(6, int(values[3] * 100));   // Address 16: OLED State

  EEPROM.commit();
  EEPROM.end();
}
void handleInputs()
{
  // --- Handle Encoder Rotation ---
  aVal = digitalRead(pinA);
  if (aVal != pinALast)
  {
    bool clockwise = (digitalRead(pinB) != aVal);

    if (currentPage == 1)
    { // On the Main Menu -> scroll through items
      menuIndex += clockwise ? 1 : -1;
      if (menuIndex < 0)
        menuIndex = TOTAL_MENU_ITEMS - 1;
      if (menuIndex >= TOTAL_MENU_ITEMS)
        menuIndex = 0;
    }
    else if (currentPage == 2)
    { // On the Adjust screen -> change the value
      values[currentMenu] += clockwise ? 0.1 : -0.1;
      if (values[currentMenu] < 0)
        values[currentMenu] = 0;
    }
    needsRedraw = true; // Flag that the screen needs an update
  }
  pinALast = aVal;

  // --- Handle Button Press ---
  if (digitalRead(sw) == LOW)
  {
    delay(200); // Simple debounce

    switch (currentPage)
    {
    case 0: // From Home screen -> go to Menu
      currentPage = 1;
      break;
    case 1: // From Menu screen -> check selection
      // *** MODIFIED LOGIC HERE ***
      if (menuIndex == TOTAL_MENU_ITEMS - 1)
      {                    // If "Save and Exit" is selected
        WriteValue();      // Save values to EEPROM
        oledstate = false; // Go back to the Home screen
        oled.displayOff();

        // NOTE: If you were saving to EEPROM, you would add that code here.
      }
      else
      {                          // If any other item is selected
        currentMenu = menuIndex; // Lock in the item to adjust
        currentPage = 2;         // Go to the Adjust Value screen
      }
      break;
    case 2: // From Adjust screen -> go back to Home screen
      currentPage = 0;
      break;
    }
    needsRedraw = true; // Flag that the screen needs an update
  }
}

/**
 * @brief Central hub for drawing; selects which screen to draw based on currentPage.
 */
void updateDisplay()
{
  oled.clear();
  switch (currentPage)
  {
  case 0:
    getstatus(float(readUltrasonicCM()) * 0.01);
    drawHomeScreen(readUltrasonicCM() * 0.01, statusss); // Using dummy values for now
    break;
  case 1:
    drawMainMenu();
    break;
  case 2:
    drawAdjustValue();
    break;
  }
  oled.display();
  needsRedraw = false; // Reset the flag after drawing
}

// --- Drawing Functions ---

void drawHomeScreen(float distance, const String &status)
{
  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 0, "Live Water Status");

  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.drawString(10, 18, "Dist:");
  oled.setTextAlignment(TEXT_ALIGN_RIGHT);
  oled.drawString(118, 18, String(distance, 2) + " m");

  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.drawString(10, 36, "State:");
  oled.setTextAlignment(TEXT_ALIGN_RIGHT);
  oled.drawString(118, 36, status);

  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 52, "Press to Edit");
}

void drawMainMenu()
{
  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 0, "Main Menu");

  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  for (int i = 0; i < TOTAL_MENU_ITEMS; i++)
  {
    if (i == menuIndex)
    {
      oled.drawString(0, 12 + (i * 10), ">");
    }
    oled.drawString(10, 12 + (i * 10), menuItems[i]);
  }
}

void drawAdjustValue()
{
  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 0, menuItems[currentMenu]);

  oled.setFont(ArialMT_Plain_24); // Make the value large and clear
  oled.drawString(64, 25, String(values[currentMenu], 2) + " m");

  oled.setFont(ArialMT_Plain_10);
  oled.drawString(64, 52, "Press to Save & Exit");
}

float getValue(int index)
{
  if (index >= 0 && index < ADJUSTABLE_MENU_ITEMS)
  {
    return values[index];
  }
  return 0.0; // Return a default value if the index is invalid
}

uint8_t confirmedNbTrials = 4;

/* User application data buffer size */

// char str[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam elementum neque et dolor finibus condimentum. Pellentesque id leo ac neque ultricies scelerisque. Cras efficitur lacinia diam, eget sed.";
void prepareValidDistance(unsigned long timeout)
{

  int validDist = -1;
  unsigned long startTime = millis();
  // Wait until valid OR 5 sec timeout
  while (validDist == -1 && (millis() - startTime < timeout))
  {
    validDist = readUltrasonicCM();
    if (validDist == -1)
    {
      Serial.println("Waiting for valid ultrasonic reading...");
      // delay(200);
    }
  }

  if (validDist != -1)
  {
    Serial.print("Got first valid distance: ");
    Serial.print(validDist);
    Serial.println(" cm");
  }
  else
  {
    Serial.println("No valid reading after timeout, continuing anyway...");
  }
  Serial.println("Elapsed time (ms): " + String(millis() - startTime));
}
// ===== Function to read SR04M-2 =====

static void prepareTxFrame(uint8_t port)
{

  StaticJsonDocument<255> sensordataJson;
  sensordataJson["dt"] = readUltrasonicCM();

  sensordataJson["dr"] = int(values[0] * 100); // Drought threshold
  sensordataJson["fr"] = int(values[1] * 100); // Flood threshold
  sensordataJson["td"] = int(values[2] * 100); // Tank depth
  sensordataJson["sh"] = int(values[3] * 100); // Sensor height

  // float distance = readUltrasonicCM() * 0.01;                 // Convert cm to m // Higher number is less full

  char jsonBuffer[255];
  size_t n = serializeJson(sensordataJson, jsonBuffer);
  appDataSize = n;
  Serial.printf("Send : %s", jsonBuffer);
  Serial.printf("Size: %d\n", appDataSize);

  for (int i = 0; i < n; i++)
  {
    appData[i] = jsonBuffer[i];
  }
}

// if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ

void setup()
{
  Serial.begin(115200);

  pinMode(Trig_PIN, OUTPUT);
  pinMode(Echo_PIN, INPUT);

  oled.init();
  oled.clear();
  oled.display();
  convertEEPROM();
  pinMode(1, OUTPUT);
  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(sw, INPUT_PULLUP);
  pinMode(2, OUTPUT); // LED pin
  pinMode(3, OUTPUT); // LED pin
  prepareValidDistance(30000);
  pinALast = digitalRead(pinA);
  oledstate = 0;
  long startreadbutton = millis();
  while (millis() - startreadbutton < 10000)
  {
    digitalWrite(1, HIGH);
    if (digitalRead(sw) == LOW)
    {
      oledstate = true;
      digitalWrite(1, LOW);
      break;
    }
  }
  while (oledstate)
  {
    handleInputs();

    // 2. Redraw the screen only if there's a change
    if (needsRedraw)
    {
      updateDisplay();
    }
  }
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  digitalWrite(1, LOW);
}

void loop()
{
  switch (deviceState)
  {
  case DEVICE_STATE_INIT:
  {
#if (LORAWAN_DEVEUI_AUTO)
    LoRaWAN.generateDeveuiByChipID();
#endif
    LoRaWAN.init(loraWanClass, loraWanRegion);
    // both set join DR and DR when ADR off
    LoRaWAN.setDefaultDR(3);
    break;
  }
  case DEVICE_STATE_JOIN:
  {
    LoRaWAN.join();
    break;
  }
  case DEVICE_STATE_SEND:
  {

    prepareTxFrame(appPort);
    LoRaWAN.send();
    digitalWrite(2, HIGH);

    deviceState = DEVICE_STATE_CYCLE;
    break;
  }
  case DEVICE_STATE_CYCLE:
  {
    // Schedule next packet transmission
    txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
    LoRaWAN.cycle(txDutyCycleTime);
    deviceState = DEVICE_STATE_SLEEP;
    break;
  }
  case DEVICE_STATE_SLEEP:
  {
    digitalWrite(2, LOW);
    LoRaWAN.sleep(loraWanClass);
    break;
  }
  default:
  {
    deviceState = DEVICE_STATE_INIT;
    break;
  }
  }
}

void downlinkCallback(uint8_t *payload, uint8_t size)
{
  Serial.print("Downlink received: ");
  for (int i = 0; i < size; i++)
  {
    Serial.print(payload[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}