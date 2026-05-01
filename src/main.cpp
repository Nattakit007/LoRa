#include <Arduino.h>
#include "LoRaWan_APP.h"
#include <ArduinoJson.h>
#include "WaterMonitor.h" // Include our new header

#include "password.h"

// --- Helper Object ---
WaterMonitor monitor;



// --- LoRaWAN Settings ---


//------ your password-----------
/*

uint8_t devEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t nwkSKey[] = {0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85};
uint8_t appSKey[] = {0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67};
uint32_t devAddr = (uint32_t)0x007e6ae1;

uint16_t userChannelsMask[6] = {0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};


or use the default from heltec


*/





LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 600000;
bool overTheAirActivation = true;
bool loraWanAdr = false;
bool isTxConfirmed = true;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

// --- Payload Preparation ---
int label = 0;

static void prepareTxFrame(uint8_t port)
{
  StaticJsonDocument<255> sensordataJson;

  // Get data from our Monitor class
  int distCM = monitor.readUltrasonicCM();

  sensordataJson["id"] = label;
  sensordataJson["dt"] = distCM;
  sensordataJson["dr"] = int(monitor.getDroughtThreshold() * 100);
  sensordataJson["fr"] = int(monitor.getFloodThreshold() * 100);
  sensordataJson["td"] = int(monitor.getTankDepth() * 100);
  sensordataJson["sh"] = int(monitor.getSensorHeight() * 100);

  char jsonBuffer[255];
  size_t n = serializeJson(sensordataJson, jsonBuffer);
  appDataSize = n;

  Serial.printf("Send : %s\n", jsonBuffer);
  Serial.printf("Size: %d\n", appDataSize);

  for (int i = 0; i < n; i++)
  {
    appData[i] = jsonBuffer[i];
  }
}

// --- Setup ---
void setup()
{
  Serial.begin(115200);

 

  // Initialize Sensor, OLED, and Pins
  monitor.begin();

  // Wait for initial valid reading
  monitor.prepareValidDistance(30000);

  // Run the Menu loop (blocks for 10s waiting for button press)
  monitor.runInitialMenuLoop(10000);

  // Initialize LoRaWAN
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
}

// --- Loop ---
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
    monitor.blinkLED(LED_PIN, 50); // Blink using helper
    deviceState = DEVICE_STATE_CYCLE;
    break;
  }
  case DEVICE_STATE_CYCLE:
  {
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