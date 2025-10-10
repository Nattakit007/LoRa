// menu_system.cpp

#include "menu_system.h"
#include "HT_SSD1306Wire.h"

// Hardware and Display Initialization (unchanged)
SSD1306Wire oled(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
// Pin variables (unchanged)
int pinA, pinB, sw, pinALast, aVal;
// Menu State and Data (unchanged)
const int menuCount = 3;
int menuIndex = 0, currentPage = 0, currentMenu = 0;
String menuItems[menuCount] = {"Drought Threshold", "Flood Threshold", "Tank Depth"};
float values[menuCount] = {6.0, 6.0, 6.0};

// Forward Declarations
void drawHomeScreen(float distance, const String& status);
void drawMainMenu();
void drawAdjustValue();
void updateEncoder();

// Public Functions
void menu_setup(int encoderPinA, int encoderPinB, int switchPin) {
  pinA = encoderPinA; pinB = encoderPinB; sw = switchPin;
  Serial.begin(115200);
  oled.init();
  pinMode(pinA, INPUT); pinMode(pinB, INPUT); pinMode(sw, INPUT_PULLUP);
  pinALast = digitalRead(pinA);
  // Initial draw with placeholder data
  drawHomeScreen(0.0, "Initializing...");
}

// <-- CHANGED: Function now receives live data
void menu_loop(float currentDistance, const String& currentStatus) {
  // Always check for user input
  updateEncoder();

  // If we are on the home screen, keep it updated with live data
  if (currentPage == 0) {
    drawHomeScreen(currentDistance, currentStatus);
  }
  
  if (digitalRead(sw) == LOW) {
    delay(200);
    switch (currentPage) {
      case 0: currentPage = 1; drawMainMenu(); break;
      case 1: currentMenu = menuIndex; currentPage = 2; drawAdjustValue(); break;
      case 2: currentPage = 0; /* Will be redrawn on the next loop */ break;
    }
  }
}

float menu_getValue(int valueIndex) {
  if (valueIndex >= 0 && valueIndex < menuCount) return values[valueIndex];
  return 0.0;
}

// Private Functions

// <-- CHANGED: This function has been completely redesigned.
void drawHomeScreen(float distance, const String& status) {
  oled.clear();
  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 0, "Live Water Status");
  
  // Display Distance
  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.drawString(10, 18, "Dist:");
  oled.setTextAlignment(TEXT_ALIGN_RIGHT);
  oled.drawString(118, 18, String(distance, 2) + " m");

  // Display Status
  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.drawString(10, 36, "State:");
  oled.setTextAlignment(TEXT_ALIGN_RIGHT);
  oled.drawString(118, 36, status);

  // Instruction
  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 46, "Press to Set Thresholds");
  
  oled.display();
}

// (The rest of the private functions are unchanged)
void drawMainMenu() {
  oled.clear();
  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 0, "Set Thresholds");
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  for (int i = 0; i < menuCount; i++) {
    if (i == menuIndex) oled.drawString(0, 20 + (i * 10), ">");
    oled.drawString(10, 20 + (i * 10), menuItems[i]);
  }
  oled.display();
}
void drawAdjustValue() {
  oled.clear();
  oled.setFont(ArialMT_Plain_10);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 0, menuItems[currentMenu]);
  oled.setFont(ArialMT_Plain_16);
  oled.drawString(64, 25, String(values[currentMenu], 2));
  oled.drawString(64, 45, "m");
  oled.display();
}
void updateEncoder() {
  aVal = digitalRead(pinA);
  if (aVal != pinALast) {
    bool clockwise = (digitalRead(pinB) != aVal);
    if (currentPage == 1) { menuIndex += clockwise ? 1 : -1; }
    else if (currentPage == 2) {
      values[currentMenu] += clockwise ? 0.1 : -0.1;
      if (values[currentMenu] < 0) values[currentMenu] = 0;
    }
    if (menuIndex < 0) menuIndex = 0;
    if (menuIndex >= menuCount) menuIndex = menuCount - 1;
    if (currentPage == 1) drawMainMenu();
    else if (currentPage == 2) drawAdjustValue();
  }
  pinALast = aVal;
}