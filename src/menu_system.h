// menu_system.h

#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include <Arduino.h> // Include for String type

// This enum provides easy-to-read names for accessing the setting values.
enum MenuItem {
  DROUGHT_THRESHOLD,
  FLOOD_THRESHOLD,
  TANK_DEPTH
};

// Initializes the OLED display, encoder pins, and menu system.
void menu_setup(int encoderPinA, int encoderPinB, int switchPin);

// Handles encoder input and screen updates.
// <-- CHANGED: Now accepts live data to display on the home screen.
void menu_loop(float currentDistance, const String& currentStatus);

// Gets the current value for a setting.
float menu_getValue(int valueIndex);

#endif // MENU_SYSTEM_H