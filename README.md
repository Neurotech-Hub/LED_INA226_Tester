# LED INA226 Tester

A closed-loop current control system for testing LEDs using the INA226 power monitor and Arduino XIAO's DAC output.

## Overview

This Arduino sketch provides precise current control for LED testing applications. It uses the INA226 current sensor to measure current through a shunt resistor and adjusts a DAC output to maintain a target current level. The system supports currents up to 1.4A with automatic safety shutdown.

## Features

- **Closed-loop current control** with proportional feedback
- **Real-time monitoring** of voltage, current, and power
- **Safety protection** with automatic shutdown at maximum current
- **Serial interface** for easy target current adjustment
- **Formatted output** with automatic header reprinting
- **DAC voltage measurement** for system diagnostics

## Hardware Requirements

### Components
- **Arduino XIAO** (or compatible SAMD21 board)
- **INA226 Power Monitor** (I2C address 0x40)
- **Shunt Resistor** (0.058Ω wire shunt for 1.4A max)
- **LED** with appropriate current limiting resistor
- **FET** (for current control)
- **Power Supply** (12V recommended)

### Connections
```
Arduino XIAO:
├── A0 (DAC) → FET Gate
├── A1 (ADC) → DAC Output (for voltage measurement)
├── SDA → INA226 SDA
├── SCL → INA226 SCL
└── 3.3V → INA226 VCC

INA226:
├── V+ → Power Supply (+)
├── V- → LED Anode
├── SDA → Arduino SDA
├── SCL → Arduino SCL
└── VCC → Arduino 3.3V

Load Circuit:
Power Supply (+) → INA226 V+ → INA226 V- → LED → Current Limiting Resistor → FET Drain
                                                                              ↓
FET Source → Shunt Resistor (0.058Ω) → Power Supply (-)
```

## Installation

1. **Install Arduino IDE** (1.8.x or 2.x)
2. **Install INA226 Library**:
   - Open Arduino IDE
   - Go to Tools → Manage Libraries
   - Search for "INA226" by Rob Tillaart
   - Install the library
3. **Upload the sketch** to your Arduino XIAO
4. **Open Serial Monitor** at 115200 baud

## Usage

### Basic Operation
1. **Power up** the system
2. **Wait** for "System ready!" message
3. **Send target current** via serial (e.g., "500" for 500mA)
4. **Monitor output** in tabular format
5. **Send "0"** to turn off the LED

### Serial Commands
- **`<number>`** - Set target current in mA (0-1400)
- **`0`** - Turn off LED

### Output Format
```
DAC     DAC_V   Time    Bus_V   Shunt_mV    Current_mA  Power_mW
512     1.65    1.0     12.045  23.456      404.123     4876.234
```

### Parameters
- **DAC**: Digital value (0-1023) sent to DAC
- **DAC_V**: Measured DAC output voltage
- **Time**: System uptime in seconds
- **Bus_V**: Bus voltage across INA226
- **Shunt_mV**: Voltage drop across shunt resistor
- **Current_mA**: Measured current in milliamps
- **Power_mW**: Calculated power in milliwatts

## Configuration

### Current Limits
- **Maximum Current**: 1400mA (limited by INA226 voltage range)
- **Shunt Resistance**: 0.058Ω (wire shunt)
- **Safety Shutdown**: Automatic at 1400mA

### Control Parameters
- **Control Rate**: 10Hz
- **Proportional Gain (Kp)**: 0.1
- **Minimum Adjustment**: 1 DAC unit

### For Higher Currents
To support currents up to 2.2A, use a 0.037Ω shunt:
```cpp
const float ACTUAL_SHUNT_OHMS = 0.037;
const float MAX_CURRENT_MA = 2200.0;
// Update INA.setMaxCurrentShunt(2.2, ACTUAL_SHUNT_OHMS);
```

## Troubleshooting

### Common Issues

**"Calibration failed with error: 0x8000"**
- Shunt voltage exceeds INA226 range
- Reduce shunt resistance or maximum current
- Check shunt resistor value

**"Could not connect to INA226"**
- Check I2C wiring (SDA, SCL)
- Verify INA226 power supply
- Check I2C address (default: 0x40)

**DAC output limited to ~2.3V**
- This is normal for Arduino XIAO
- Hardware limitation of SAMD21 DAC
- Not a fault or damage

**Current readings inaccurate**
- Measure actual shunt resistance
- Update `ACTUAL_SHUNT_OHMS` constant
- Check for loose connections

### Debug Information
The system provides calibration information on startup:
```
Current LSB: 0.427246 mA
Shunt resistance: 0.058000 Ω
```

## Technical Details

### INA226 Configuration
- **I2C Address**: 0x40 (default)
- **PGA Range**: ±81.92mV (automatic)
- **Current LSB**: Calculated based on shunt and max current
- **Calibration**: Automatic via library

### Control Algorithm
Simple proportional control:
```
error = target_current - measured_current
adjustment = error * Kp
new_dac_value = current_dac_value + adjustment
```

### Safety Features
- **Overcurrent Protection**: Automatic shutdown at 1400mA
- **DAC Limits**: Constrained to 0-1023 range
- **Serial Validation**: Input range checking

## License

This project is open source. Feel free to modify and distribute.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Version History

- **v1.0** - Initial release with closed-loop current control
- Supports up to 1.4A with 0.058Ω shunt
- Real-time monitoring and safety features
