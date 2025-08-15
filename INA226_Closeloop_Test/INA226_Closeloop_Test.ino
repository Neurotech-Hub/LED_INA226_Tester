#include <INA226.h>
#include <Wire.h>

INA226 ina226(0x4A);

// DAC pin for ItsyBitsy M4
const int dacPin = A0; // DAC output pin

// Configuration
float shuntResistance = 0.04195; // 42mΩ shunt
float maxCurrent_mA = 1000;      // 1A max current to stay within 81.9mV shunt voltage limit
float current = 0.0;
float targetCurrent_mA = 0; // Target current in mA
int currentDacValue = 0;    // Current DAC value (0-1023)
const int dacMin = 0;       // Minimum DAC value
const int dacMax = 4095;    // Maximum DAC value
float voltage = 0.0;
float shuntVoltage = 0.0;

const uint16_t ALERT_CONVERSION_READY = 0x0400;

// Timing variables
unsigned long startTime = 0;
unsigned long endTime = 0;
unsigned long readTime = 0;

void setup()
{
  Serial.begin(115200);

  // Initialize I2C
  Wire.begin();

  // Initialize INA226
  if (initializeINA226())
  {
    Serial.println("INA226 initialized successfully!");
  }
  else
  {
    Serial.println("ERROR: Failed to initialize INA226!");
    Serial.println("Check connections and I2C address");
  }

  Serial.println();
}

void loop()
{
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Try to parse as current value
    float newTarget = input.toFloat();
    if (newTarget >= 0 && newTarget <= maxCurrent_mA)
    {
      targetCurrent_mA = newTarget;
      if (targetCurrent_mA == 0)
      {
        currentDacValue = 0;
        analogWrite(dacPin, currentDacValue);
      }
      else
      {
        Serial.print("Target current set to: ");
        Serial.print(targetCurrent_mA);
        Serial.println(" mA");
      }
    }
  }

  // Wait for conversion to be ready (timeout after 600ms)
  if (!ina226.waitConversionReady())
  {
    Serial.println("Timeout waiting for conversion!");
    return;
  }

  if (targetCurrent_mA > 0)
  {
    startTime = micros();
    shuntVoltage = ina226.getShuntVoltage_mV();
    voltage = ina226.getBusVoltage();
    current = ina226.getCurrent_mA();
    endTime = micros();
    readTime = endTime - startTime;

    currentDacValue += (current < targetCurrent_mA) ? 1 : -1;
    currentDacValue = constrain(currentDacValue, dacMin, dacMax);
    analogWrite(dacPin, currentDacValue);

    Serial.print("DAC Value: ");
    Serial.println(currentDacValue);
    Serial.print("Current (mA): ");
    Serial.println(current);
    Serial.print("Bus Voltage (V): ");
    Serial.println(voltage);
    Serial.print("Shunt Voltage (mV): ");
    Serial.println(shuntVoltage);
    Serial.print("Read Time (us): ");
    Serial.println(readTime);
  }
  else
  {
    analogWrite(dacPin, 0);
  }
}

// Initialize INA226
bool initializeINA226()
{
  Serial.println("Initializing INA226...");

  if (!ina226.begin())
  {
    return false;
  }

  // Configure INA226
  ina226.setAverage();
  ina226.setBusVoltageConversionTime();
  ina226.setShuntVoltageConversionTime();
  ina226.setMode();

  // Calculate and verify calibration
  if (!calculateCalibration())
  {
    return false;
  }

  Serial.println("INA226 configured successfully");
  Serial.println("Current LSB: " + String(ina226.getCurrentLSB_mA(), 6) + " mA");
  return true;
}

bool calculateCalibration()
{
  Serial.println("Calculating calibration...");
  Serial.println("Shunt resistance: " + String(shuntResistance, 3) + " Ω");

  // Convert mA to A for calibration
  int result = ina226.setMaxCurrentShunt(maxCurrent_mA / 1000.0, shuntResistance);

  if (result == INA226_ERR_NONE)
  {
    Serial.println("Calibration successful!");
    Serial.println("Current LSB: " + String(ina226.getCurrentLSB_mA(), 6) + " mA");
    Serial.println("Shunt: " + String(ina226.getShunt(), 3) + " Ω");
    Serial.println("Max Current: " + String(ina226.getMaxCurrent(), 2) + " A");
    return true;
  }
  else
  {
    Serial.println("ERROR: Calibration failed with code: 0x" + String(result, HEX));
    switch (result)
    {
    case INA226_ERR_SHUNTVOLTAGE_HIGH:
      Serial.println("Error: maxCurrent * shunt > 81.9 mV");
      Serial.println("Reduce maxCurrent or use smaller shunt resistance");
      break;
    case INA226_ERR_MAXCURRENT_LOW:
      Serial.println("Error: maxCurrent < 0.001 A");
      Serial.println("Increase maxCurrent value");
      break;
    case INA226_ERR_SHUNT_LOW:
      Serial.println("Error: shunt resistance < 0.001 Ω");
      Serial.println("Increase shunt resistance value");
      break;
    case INA226_ERR_NORMALIZE_FAILED:
      Serial.println("Error: normalization failed");
      Serial.println("Try setting normalize=false in setMaxCurrentShunt()");
      break;
    default:
      Serial.println("Unknown error code");
      break;
    }
    return false;
  }
}
