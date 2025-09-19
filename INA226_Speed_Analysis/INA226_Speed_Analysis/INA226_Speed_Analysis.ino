#include <Wire.h>
#include <INA226.h>

// INA226 Configuration
INA226 ina(0x4A);  // Constructor with address parameter
const uint8_t INA226_ADDRESS = 0x4A;  // I2C address

// Measurement Parameters
const float SHUNT_RESISTANCE = 0.04195;   // shunt resistor
const int AVERAGING_SAMPLES = 4;      
const int MAX_CURRENT = 1.6;          // Maximum expected current (A)

// DAC pin for ItsyBitsy M4
const int dacPin = A0; // DAC output pin

// Timing Configuration Options (using library constants)
enum ConversionTime {
  CONV_140US = 0,   // INA226_140_us
  CONV_204US = 1,   // INA226_204_us
  CONV_332US = 2,   // INA226_332_us
  CONV_588US = 3,   // INA226_588_us
  CONV_1100US = 4,  // INA226_1100_us (default)
  CONV_2116US = 5,  // INA226_2100_us
  CONV_4156US = 6,  // INA226_4200_us
  CONV_8244US = 7   // INA226_8300_us
};

// Averaging options (using library constants)
enum AveragingSamples {
  AVG_1 = 0,     // INA226_1_SAMPLE
  AVG_4 = 1,     // INA226_4_SAMPLES  
  AVG_16 = 2,    // INA226_16_SAMPLES
  AVG_64 = 3,    // INA226_64_SAMPLES
  AVG_128 = 4,   // INA226_128_SAMPLES
  AVG_256 = 5,   // INA226_256_SAMPLES
  AVG_512 = 6,   // INA226_512_SAMPLES
  AVG_1024 = 7   // INA226_1024_SAMPLES
};

// Current configuration - easily adjustable
ConversionTime shuntConvTime = CONV_588US;  // Good balance of speed/precision
ConversionTime busConvTime = CONV_140US;    // Fast bus conversion for optimization
AveragingSamples averagingSetting = AVG_4;  // 4 samples averaging

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Set DAC resolution to 12-bit
  analogWriteResolution(12);

  // Initialize INA226
  if (!ina.begin()) {
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
  Serial.println("Enter 'dac,xxx' to set DAC value (0-4095).");
  Serial.println("Enter 'm' or 'measure' to take a measurement.");
  Serial.println("Enter 'test' to run the performance test.");
}

void configureINA226() {
  // Set calibration for current calculation
  // For RobTillaart library, use setMaxCurrentShunt method
  bool calibrated = ina.setMaxCurrentShunt(MAX_CURRENT, SHUNT_RESISTANCE);
  if (!calibrated) {
    Serial.println("Calibration failed!");
  }
  
  // Configure averaging and conversion times using correct function names
  ina.setAverage(averagingSetting);  // Correct function name
  ina.setShuntVoltageConversionTime(shuntConvTime);  // Correct function name  
  ina.setBusVoltageConversionTime(busConvTime);
  
  // Set to continuous mode for fastest operation
  // Note: RobTillaart library uses different constants
  // Default is continuous mode, so this might not be needed
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("dac,")) {
      int dacValue = command.substring(4).toInt();
      dacValue = constrain(dacValue, 0, 4095);
      analogWrite(dacPin, dacValue);
      Serial.print("DAC set to: ");
      Serial.println(dacValue);
    } else if (command == "m" || command == "measure") {
      performMeasurement();
    } else if (command == "test") {
      runTest();
    } else {
      Serial.println("Unknown command.");
    }
  }
}

void performMeasurement() {
  // Wait for conversion to be ready using correct function name
  if (!ina.isConversionReady()) {
    Serial.println("Conversion not ready, waiting...");
    // Alternative: use blocking wait
    // ina.waitUntilConversionCompleted();
    return;
  }

  // Now we know conversion is ready, read the current
  unsigned long startTime = micros();
  float current = readCurrentOptimized();
  unsigned long measurementTime = micros() - startTime;

  // Display results
  Serial.print("Current: ");
  Serial.print(current*1000, 2);
  Serial.println(" mA");
  
  // Show timing information
  Serial.print("Measurement time: ");
  Serial.print(measurementTime);
  Serial.println(" µs");
  
  // Optional: Show shunt voltage
  float shuntVoltage = ina.getShuntVoltage();  // Returns in Volts
  Serial.print("Shunt Voltage: ");
  Serial.print(shuntVoltage, 6);
  Serial.println(" V");
}

float readCurrentOptimized() {
  // Direct current reading using library's built-in calculation
  return ina.getCurrent();  // Returns current in Amperes directly
}

float calculateCurrentFromShunt() {
  // Alternative method: Calculate current from shunt voltage
  float shuntVoltage = ina.getShuntVoltage();  // Already in Volts
  return shuntVoltage / SHUNT_RESISTANCE;
}

// Utility function to change configuration on-the-fly
void updateConfiguration(ConversionTime newShuntTime, AveragingSamples newAveraging) {
  shuntConvTime = newShuntTime;
  averagingSetting = newAveraging;
  
  ina.setShuntVoltageConversionTime(shuntConvTime);  // Correct function name
  ina.setAverage(newAveraging);  // Correct function name
  
  Serial.print("Updated configuration - Averaging: ");
  Serial.print(getAveragingSamplesCount(newAveraging));
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
    case CONV_4156US: return "4200µs";
    case CONV_8244US: return "8300µs";
    default: return "Unknown";
  }
}

int getAveragingSamplesCount(AveragingSamples avgSetting) {
  switch(avgSetting) {
    case AVG_1: return 1;
    case AVG_4: return 4;
    default: return 1;
  }
}

// Function to calculate expected measurement time
float calculateMeasurementTime() {
  float conversionTimes[] = {140, 204, 332, 588, 1100, 2116, 4200, 8300};
  float shuntTime = conversionTimes[shuntConvTime];
  float busTime = conversionTimes[busConvTime];
  int sampleCount = getAveragingSamplesCount(averagingSetting);
  
  // Total time = (Averages × Shunt Time) + (Averages × Bus Time)
  return (sampleCount * shuntTime) + (sampleCount * busTime);
}

void runTest() {
  Serial.println("\n--- Starting Measurement Test ---");
  Serial.println("Settings            | getCurrent()");
  Serial.println("----------------------------------------");

  // Arrays of settings to test
  ConversionTime convTimes[] = {
    CONV_140US, CONV_204US, CONV_332US, CONV_588US, 
    CONV_1100US};
  AveragingSamples avgSamples[] = {AVG_1, AVG_4};

  for (int i = 0; i < 5; i++) {
    ConversionTime convTime = convTimes[i];
    for (int j = 0; j < 2; j++) {
      AveragingSamples avgSample = avgSamples[j];

      // Update configuration on the INA226
      updateConfiguration(convTime, avgSample);

      unsigned long startTime1 = micros();

      // Wait for data to be ready
      while (!ina.isConversionReady());

      // Time the readCurrentOptimized() method
      float current = readCurrentOptimized();
      unsigned long time1 = micros() - startTime1;
      
      // Print formatted results
      String settingStr = getConversionTimeString(convTime);
      settingStr += ", " + String(getAveragingSamplesCount(avgSample)) + " avg";
      Serial.print(settingStr);
      for (int k = settingStr.length(); k < 20; k++) Serial.print(" ");

      Serial.print("| ");
      Serial.print(time1);
      Serial.print("µs");
      for (int k = String(time1).length(); k < 20; k++) Serial.print(" ");
      Serial.print("| ");
      Serial.print(current*1000,2);
      Serial.println(" mA");
    }
  }
  Serial.println("--- Test Complete ---");
}
