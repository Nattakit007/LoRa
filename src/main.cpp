
#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include "menu_system.h"
#define Trig_PIN 47
#define Echo_PIN 48

// ---------------- Encoder AND Oled ----------------



// ========== Ultrasonic pins ==========

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
uint32_t appTxDutyCycle = 60000;

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
uint8_t confirmedNbTrials = 4;

/* User application data buffer size */

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

  if (duration > 0)
  {
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
      delay(200);
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
}
// ===== Function to read SR04M-2 =====


static void prepareTxFrame(uint8_t port)
{

  StaticJsonDocument<255> sensordataJson;
  sensordataJson["distance_cm"] = readUltrasonicCM();

  // sensordataJson["ldr_value"] = analogRead(LDR_PIN);

  char jsonBuffer[255];
  size_t n = serializeJson(sensordataJson, jsonBuffer);

  // Serial.printf("JsonData :%s \n",sensordataJson);
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
  menu_setup(41,48,19);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  pinMode(Trig_PIN, OUTPUT); // Trig pin will send a pulse
  pinMode(Echo_PIN, INPUT);  // Echo pin will listen for the return pulse
  prepareValidDistance(30000);
}

void loop()
{
  // float distance = readUltrasonicCM() * 0.01; // Convert cm to m
  float distance = 6.0 + 4.0 * sin(millis() / 2000.0);
  float flood_threshold = menu_getValue(FLOOD_THRESHOLD); // Lower number is more full
  float drought_threshold = menu_getValue(DROUGHT_THRESHOLD); // Higher number is less full
  String status = "Normal";
  if (distance < flood_threshold) {
    status = "Flood";
  } else if (distance > drought_threshold) {
    status = "Drought";
  }
  menu_loop(distance, status);
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