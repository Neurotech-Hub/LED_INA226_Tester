#include <Wire.h>
#include <INA226.h>

// INA226 Configuration
INA226 ina;
const uint8_t INA226_ADDRESS = 0x40;  // Default I2C address

// Measurement Parameters
const float SHUNT_RESISTANCE = 0.04195;   // 100mΩ shunt resistor
const int AVERAGING_SAMPLES = 4;      
const int MAX_CURRENT = 1.6;          // Maximum expected current (A)

// Timing Configuration Options
enum ConversionTime {
  CONV_140US = 0,   
  CONV_204US = 1,
  CONV_332US = 2,
  CONV_588US = 3,
  CONV_1100US = 4,   
  CONV_2116US = 5,
  CONV_4156US = 6,
  CONV_8244US = 7   
};

// Current configuration - easily adjustable
ConversionTime shuntConvTime = CONV_588US;  // Good balance of speed/precision
ConversionTime busConvTime = CONV_140US;    // Fast bus conversion for optimization

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Initialize INA226
  if (!ina.begin(INA226_ADDRESS)) {
    Serial.println("Failed to initialize INA226!");
    while(1);
  }
  
  // Configure INA226 for optimized current measurement
  configureINA226();
  
  Serial.println("INA226 Current Monitor Initialized");
  Serial.println("Configuration:");
  Serial.print("- Averaging: "); Serial.print(AVERAGING_SAMPLES); Serial.println(" samples");
  Serial.print("- Shunt Resistance: "); Serial.print(SHUNT_RESISTANCE, 3); Serial.println(" Ω");
  Serial.print("- Max Current: "); Serial.print(MAX_CURRENT); Serial.println(" A");
  Serial.println("- Optimized for current-only measurements");
  Serial.println();
}

void configureINA226() {
  // Set calibration for current calculation
  // Calibration = 0.00512 / (Current_LSB * Rshunt)
  // Current_LSB = Max_Current / 32768
  float currentLSB = MAX_CURRENT / 32768.0;
  uint16_t calibration = (uint16_t)(0.00512 / (currentLSB * SHUNT_RESISTANCE));
  
  ina.setCalibration(calibration);
  
  // Configure averaging and conversion times
  // Format: setAveraging(samples), setShuntConversionTime(time), setBusConversionTime(time)
  ina.setAveraging(AVERAGING_SAMPLES);
  ina.setShuntConversionTime(shuntConvTime);
  ina.setBusConversionTime(busConvTime);    // Optimized for speed since we focus on current
  
  // Set to continuous mode for fastest operation
  ina.setMode(INA226_MODE_CONTINUOUS);
}

void loop() {
  // Wait for conversion to be ready (with timeout protection)
  if (!ina.waitConversionReady()) {
    Serial.println("Timeout waiting for conversion!");
    return;  // Skip this iteration if conversion not ready
  }

  // Now we know conversion is ready, read the current
  unsigned long startTime = micros();
  float current = readCurrentOptimized();
  unsigned long measurementTime = micros() - startTime;

  // Display results
  Serial.print("Current: ");
  Serial.print(current, 4);
  Serial.println(" A");
  
  // Show timing information
  Serial.print("Measurement time: ");
  Serial.print(measurementTime);
  Serial.println(" µs");
  
  // float shuntVoltage = ina.readShuntVoltage() / 1000000.0;  // Convert µV to V
  // Serial.print("Shunt Voltage: ");
  // Serial.print(shuntVoltage, 6);
  // Serial.println(" V");
}
  
}

float readCurrentOptimized() {
  // Direct current reading using library's built-in calculation
  return ina.readCurrent() / 1000.0;  // Convert mA to A
}

float calculateCurrentFromShunt() {
  // Alternative method: Calculate current from shunt voltage
  // This gives you more control over the calculation
  float shuntVoltage = ina.readShuntVoltage() / 1000000.0;  // Convert µV to V
  return shuntVoltage / SHUNT_RESISTANCE;
}

// Utility function to change configuration on-the-fly
void updateConfiguration(ConversionTime newShuntTime, int newAveraging) {
  shuntConvTime = newShuntTime;
  ina.setShuntConversionTime(shuntConvTime);
  ina.setAveraging(newAveraging);
  
  Serial.print("Updated configuration - Averaging: ");
  Serial.print(newAveraging);
  Serial.print(" samples, Conversion time: ");
  Serial.println(getConversionTimeString(newShuntTime));
}

String getConversionTimeString(ConversionTime convTime) {
  switch(convTime) {
    case CONV_140US: return "140µs";
    case CONV_204US: return "204µs";
    case CONV_332US: return "332µs";
    case CONV_588US: return "588µs";
    case CONV_1100US: return "1100µs";
    case CONV_2116US: return "2116µs";
    case CONV_4156US: return "4156µs";
    case CONV_8244US: return "8244µs";
    default: return "Unknown";
  }
}

// Function to calculate expected measurement time
float calculateMeasurementTime() {
  float conversionTimes[] = {140, 204, 332, 588, 1100, 2116, 4156, 8244};
  float shuntTime = conversionTimes[shuntConvTime];
  float busTime = conversionTimes[busConvTime];
  
  // Total time = (Averages × Shunt Time) + (Averages × Bus Time)
  // But since we're optimizing for current, bus time impact is minimal
  return (AVERAGING_SAMPLES * shuntTime) + (AVERAGING_SAMPLES * busTime);
}
