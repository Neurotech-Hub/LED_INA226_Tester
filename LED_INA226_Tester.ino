//
//    FILE: LED_INA226_Tester.ino
//  PURPOSE: Test LEDs using INA226 power monitor with current-controlled DAC
//  SETUP: Connect INA226 to Arduino via I2C
//         Connect LED and current limiting resistor to load terminals
//         Connect DAC output to A0

#include "INA226.h"

// INA226 instance - default I2C address is 0x40
INA226 INA(0x40);

// DAC pin
const int DAC_PIN = A0;
const int DAC_MEASURE_PIN = A1; // Use A1 to measure DAC output voltage
bool headerPrinted = false;
int lineCount = 0; // Counter for header reprinting

// Control parameters
float targetCurrent_mA = 0;                                 // Target current in mA
int currentDacValue = 0;                                    // Current DAC value (0-1023)
const int DAC_MIN = 0;                                      // Minimum DAC value
const int DAC_MAX = 1023;                                   // Maximum DAC value
const float CONTROL_RATE = 10;                              // Control loop rate in Hz
const unsigned long CONTROL_INTERVAL = 1000 / CONTROL_RATE; // Control interval in ms
unsigned long lastControlTime = 0;
const float MAX_CURRENT_MA = 1400.0; // Absolute maximum current in mA (limited by INA226 voltage range)

// Control gains
const float Kp = 0.1;             // Proportional gain
const float MIN_ADJUSTMENT = 1.0; // Minimum DAC adjustment

// Actual shunt resistance (adjust this based on your wire measurements)
const float ACTUAL_SHUNT_OHMS = 0.058; // Calculated from 1400mA reading vs 1200mA actual

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ; // Wait for serial port to connect
    }
    Serial.println("LED INA226 Power Tester with Current Control");
    Serial.println("==========================================");
    Serial.println("Send target current in mA (0-1500) to control LED");
    Serial.println();

    // Configure DAC pin
    pinMode(DAC_PIN, OUTPUT);
    analogWriteResolution(10); // Set to 10-bit resolution for XIAO

    // Configure measurement pin
    pinMode(DAC_MEASURE_PIN, INPUT);

    Wire.begin();
    if (!INA.begin())
    {
        Serial.println("Could not connect to INA226. Please check wiring.");
        while (1)
            ; // Stop if we can't connect
    }

    // Configure for actual shunt resistance and max 1.4A current (to stay within ±81.92mV range)
    int result = INA.setMaxCurrentShunt(1.4, ACTUAL_SHUNT_OHMS);

    if (result != 0)
    {
        Serial.print("Calibration failed with error: 0x");
        Serial.println(result, HEX);
        while (1)
            ; // Stop if calibration fails
    }

    Serial.print("Current LSB: ");
    Serial.println(INA.getCurrentLSB_mA(), 6);
    Serial.print("Shunt resistance: ");
    Serial.println(INA.getShunt(), 6);
    Serial.println("System ready!");
    Serial.println();
}

void loop()
{
    // Check for serial commands
    if (Serial.available())
    {
        String input = Serial.readStringUntil('\n');
        input.trim();

        // Try to parse as current value
        float newTarget = input.toFloat();
        if (newTarget >= 0 && newTarget <= MAX_CURRENT_MA)
        {
            targetCurrent_mA = newTarget;
            if (targetCurrent_mA == 0)
            {
                currentDacValue = 0;
                analogWrite(DAC_PIN, currentDacValue);
                Serial.println("LED turned off");
            }
            else
            {
                Serial.print("Target current set to: ");
                Serial.print(targetCurrent_mA);
                Serial.println(" mA");
            }
        }
        else
        {
            Serial.print("Invalid current value. Must be 0-");
            Serial.print(MAX_CURRENT_MA);
            Serial.println(" mA");
        }
    }

    // Control loop
    if (millis() - lastControlTime >= CONTROL_INTERVAL)
    {
        lastControlTime = millis();

        if (targetCurrent_mA > 0)
        {
            float currentCurrent = INA.getCurrent_mA();

            // Safety check - if current exceeds max, shut down immediately
            if (currentCurrent > MAX_CURRENT_MA)
            {
                targetCurrent_mA = 0;
                currentDacValue = 0;
                analogWrite(DAC_PIN, currentDacValue);
                Serial.println("SAFETY SHUTDOWN: Current exceeded maximum limit!");
                return;
            }

            // Calculate error and adjust DAC
            float error = targetCurrent_mA - currentCurrent;
            int adjustment = (int)(error * Kp);

            if (abs(adjustment) >= MIN_ADJUSTMENT)
            {
                currentDacValue += adjustment;
                currentDacValue = constrain(currentDacValue, DAC_MIN, DAC_MAX);
                analogWrite(DAC_PIN, currentDacValue);
            }

            // Print measurements
            if (!headerPrinted || lineCount % 10 == 0)
            {
                Serial.println("DAC\tDAC_V\tTime\tBus_V\tShunt_mV\tCurrent_mA\tPower_mW");
                headerPrinted = true;
            }

            float dacVoltage = measureDacVoltage();
            Serial.print(currentDacValue);
            Serial.print("\t");
            Serial.print(dacVoltage, 2);
            Serial.print("\t");
            Serial.print(millis() / 1000.0, 1);
            Serial.print("\t");
            Serial.print(INA.getBusVoltage(), 3);
            Serial.print("\t");
            Serial.print(INA.getShuntVoltage_mV(), 3);
            Serial.print("\t");
            Serial.print(currentCurrent, 3);
            Serial.print("\t");
            Serial.println(INA.getPower_mW(), 3);

            lineCount++; // Increment line counter
        }
    }
}

float measureDacVoltage()
{
    // Read the DAC voltage using A1 pin
    int adcValue = analogRead(DAC_MEASURE_PIN);
    // Convert to voltage (assuming 3.3V reference)
    float voltage = (adcValue * 3.3) / 1023.0;
    return voltage;
}