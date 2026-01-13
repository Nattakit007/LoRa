#ifndef WATERMONITOR_H
#define WATERMONITOR_H

#include <Arduino.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <EEPROM.h>
#include <AsyncDelay.h>
#include <jsnsr04t.h>

// --- Hardware Constants ---
#define TRIG_PIN 46
#define ECHO_PIN 45
#define PIN_A    36 // Encoder CLK
#define PIN_B    37 // Encoder DT
#define PIN_SW   38 // Encoder Switch
#define LED_PIN  2
#define EXT_LED  3

// --- Menu Constants ---
#define ADJUSTABLE_MENU_ITEMS 4
#define TOTAL_MENU_ITEMS 5
#define EEPROM_SIZE 8



class WaterMonitor {
  public:
    WaterMonitor(); // Constructor

    // Setup functions
    void begin();
    void runInitialMenuLoop(unsigned long timeoutMs);
    
    // Sensor functions
    int readUltrasonicCM();
    void prepareValidDistance(unsigned long timeout);
    String getStatusString(float distanceM);
    
    // Data getters for LoRa transmission
    float getDroughtThreshold();
    float getFloodThreshold();
    float getTankDepth();
    float getSensorHeight();
    
    // Debug helpers
    void blinkLED(int pin, int duration);

  private:
    SSD1306Wire* oled;

    float values[ADJUSTABLE_MENU_ITEMS]; 
    String menuItems[TOTAL_MENU_ITEMS];
    
    // Menu State
    int currentPage;
    int menuIndex;
    int currentMenu;
    bool needsRedraw;
    bool oledActive;

    // Encoder State
    int pinALast;
    int aVal;

    // Internal helper functions
    void handleInputs();
    void updateDisplay();
    void drawHomeScreen(float distance, const String &status);
    void drawMainMenu();
    void drawAdjustValue();
    void loadFromEEPROM();
    void saveToEEPROM();
};

#endif