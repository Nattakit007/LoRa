#include "WaterMonitor.h"
// --- WaterMonitor obj ---
JsnSr04T ultrasonicSensor(ECHO_PIN, TRIG_PIN, LOG_LEVEL_VERBOSE);
AsyncDelay measureDelay;

WaterMonitor::WaterMonitor()
{
    // Initialize objects
    oled = new SSD1306Wire(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

    // Initialize Defaults
    currentPage = 0;
    menuIndex = 0;
    currentMenu = 0;
    needsRedraw = true;
    oledActive = true;

    menuItems[0] = "Drought Threshold";
    menuItems[1] = "Flood Threshold";
    menuItems[2] = "Tank Depth";
    menuItems[3] = "Sensor High";
    menuItems[4] = "Save and Exit";

    // Default values before EEPROM load
    values[0] = 6.0;
    values[1] = 6.0;
    values[2] = 6.0;
    values[3] = 6.0;
}

void WaterMonitor::begin()
{
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(PIN_A, INPUT);
    pinMode(PIN_B, INPUT);
    pinMode(PIN_SW, INPUT_PULLUP);
    pinMode(1, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(EXT_LED, OUTPUT);

    // Begin Ultrasonic Sensor
    

    oled->init();
    oled->clear();
    oled->display();

    pinALast = digitalRead(PIN_A);
    loadFromEEPROM();
}

void WaterMonitor::loadFromEEPROM()
{
    EEPROM.begin(EEPROM_SIZE);
    // Using simple multiply/divide to store float as int (x100) if needed,
    // or just reading byte-wise if that was your intention.
    // Based on your original code logic:
    values[0] = EEPROM.read(0) * 0.01;
    values[1] = EEPROM.read(2) * 0.01;
    values[2] = EEPROM.read(4) * 0.01;
    values[3] = EEPROM.read(6) * 0.01;
    EEPROM.end();
}

void WaterMonitor::saveToEEPROM()
{
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(0, int(values[0] * 100));
    EEPROM.write(2, int(values[1] * 100));
    EEPROM.write(4, int(values[2] * 100));
    EEPROM.write(6, int(values[3] * 100));
    EEPROM.commit();
    EEPROM.end();
}

int WaterMonitor::readUltrasonicCM()
{

    if (measureDelay.isExpired())
    {
        int distance = ultrasonicSensor.readDistance();
        Serial.print("Distance: ");
        Serial.print(distance);
        measureDelay.repeat();
    }
}

void WaterMonitor::prepareValidDistance(unsigned long timeout)
{
    int validDist = -1;
    unsigned long startTime = millis();
    while (validDist == -1 && (millis() - startTime < timeout))
    {
        validDist = readUltrasonicCM();
        if (validDist == -1)
            Serial.println("Waiting for sensor...");
    }
    Serial.print("Initial Dist: ");
    Serial.println(validDist);
}

// This runs the blocking loop for the menu at startup
void WaterMonitor::runInitialMenuLoop(unsigned long timeoutMs)
{
    oledActive = false;
    long startWait = millis();

    // Wait for button press to enter menu
    while (millis() - startWait < timeoutMs)
    {
        digitalWrite(1, HIGH);
        if (digitalRead(PIN_SW) == LOW)
        {
            oledActive = true;
            digitalWrite(1, LOW);
            break;
        }
        delay(10);
    }

    digitalWrite(1, LOW);

    // If menu activated, stay here until exit
    while (oledActive)
    {
        handleInputs();
        if (needsRedraw)
        {
            updateDisplay();
        }
        // Small delay to prevent watchdog crash
        delay(5);
    }
    oled->displayOff();
}

void WaterMonitor::handleInputs()
{
    // Encoder Rotation
    aVal = digitalRead(PIN_A);
    if (aVal != pinALast)
    {
        bool clockwise = (digitalRead(PIN_B) != aVal);

        if (currentPage == 1)
        {
            menuIndex += clockwise ? 1 : -1;
            if (menuIndex < 0)
                menuIndex = TOTAL_MENU_ITEMS - 1;
            if (menuIndex >= TOTAL_MENU_ITEMS)
                menuIndex = 0;
        }
        else if (currentPage == 2)
        {
            values[currentMenu] += clockwise ? 0.1 : -0.1;
            if (values[currentMenu] < 0)
                values[currentMenu] = 0;
        }
        needsRedraw = true;
    }
    pinALast = aVal;

    // Button Press
    if (digitalRead(PIN_SW) == LOW)
    {
        delay(200); // Debounce
        switch (currentPage)
        {
        case 0:
            currentPage = 1;
            break;
        case 1:
            if (menuIndex == TOTAL_MENU_ITEMS - 1)
            { // Save Exit
                saveToEEPROM();
                oledActive = false;
            }
            else
            {
                currentMenu = menuIndex;
                currentPage = 2;
            }
            break;
        case 2:
            currentPage = 0;
            break;
        }
        needsRedraw = true;
    }
}

void WaterMonitor::updateDisplay()
{
    oled->clear();
    switch (currentPage)
    {
    case 0:
        drawHomeScreen(readUltrasonicCM() * 0.01, getStatusString(readUltrasonicCM() * 0.01));
        break;
    case 1:
        drawMainMenu();
        break;
    case 2:
        drawAdjustValue();
        break;
    }
    oled->display();
    needsRedraw = false;
}

String WaterMonitor::getStatusString(float distance)
{
    if (distance < values[0])
        return "Drought";
    else if (distance > values[1])
        return "Flood";
    return "Normal";
}

void WaterMonitor::drawHomeScreen(float distance, const String &status)
{
    oled->setFont(ArialMT_Plain_10);
    oled->setTextAlignment(TEXT_ALIGN_CENTER);
    oled->drawString(64, 0, "Live Water Status");
    oled->setFont(ArialMT_Plain_16);
    oled->setTextAlignment(TEXT_ALIGN_LEFT);
    oled->drawString(10, 18, "Dist:");
    oled->setTextAlignment(TEXT_ALIGN_RIGHT);
    oled->drawString(118, 18, String(distance, 2) + " m");
    oled->setFont(ArialMT_Plain_10);
    oled->setTextAlignment(TEXT_ALIGN_LEFT);
    oled->drawString(10, 36, "State:");
    oled->setTextAlignment(TEXT_ALIGN_RIGHT);
    oled->drawString(118, 36, status);
    oled->setTextAlignment(TEXT_ALIGN_CENTER);
    oled->drawString(64, 52, "Press to Edit");
}

void WaterMonitor::drawMainMenu()
{
    oled->setFont(ArialMT_Plain_10);
    oled->setTextAlignment(TEXT_ALIGN_CENTER);
    oled->drawString(64, 0, "Main Menu");
    oled->setTextAlignment(TEXT_ALIGN_LEFT);
    for (int i = 0; i < TOTAL_MENU_ITEMS; i++)
    {
        if (i == menuIndex)
            oled->drawString(0, 12 + (i * 10), ">");
        oled->drawString(10, 12 + (i * 10), menuItems[i]);
    }
}

void WaterMonitor::drawAdjustValue()
{
    oled->setFont(ArialMT_Plain_10);
    oled->setTextAlignment(TEXT_ALIGN_CENTER);
    oled->drawString(64, 0, menuItems[currentMenu]);
    oled->setFont(ArialMT_Plain_24);
    oled->drawString(64, 25, String(values[currentMenu], 2) + " m");
    oled->setFont(ArialMT_Plain_10);
    oled->drawString(64, 52, "Press to Save & Exit");
}

float WaterMonitor::getDroughtThreshold() { return values[0]; }
float WaterMonitor::getFloodThreshold() { return values[1]; }
float WaterMonitor::getTankDepth() { return values[2]; }
float WaterMonitor::getSensorHeight() { return values[3]; }
void WaterMonitor::blinkLED(int pin, int duration)
{
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
}