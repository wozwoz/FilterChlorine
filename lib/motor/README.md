# Motor Driver Library

This library supports multiple motor driver types for ESP32:

## Supported Drivers

### MD135 (2-pin driver)
- **Pins**: PWM, DIR
- **Control**: 1 PWM speed pin + 1 direction pin
- **High power motor driver**
- **2-wire control interface**

### MD13S (2-pin driver)
- **Pins**: PWM, DIR
- **Control**: 1 direction pin + 1 PWM speed pin
- **Current**: 13A continuous, 30A peak
- **Single motor only**

### Legacy: L298N/L293D (3-pin H-bridge)
- **Pins**: IN1, IN2, ENABLE
- **Control**: 2 direction pins + 1 PWM speed pin
- **Not supported by Motor/MD135 classes**
- **Use custom implementation if needed**

## Usage Examples

### Using MD135 (2-wire control)

```cpp
#include "md135.h"

// Create motor instance
// MD135(PWM_pin, DIR_pin, PWM_channel, frequency, resolution)
MD135 motor(25, 26, 0, 5000, 8);

void setup() {
    motor.begin();  // Initialize pins and PWM
}

void loop() {
    // Move forward at half speed
    motor.forward(127);
    delay(2000);
    
    // Stop
    motor.stop();
    delay(1000);
    
    // Reverse at full speed
    motor.reverse(255);
    delay(2000);
    
    // Stop
    motor.stop();
    delay(1000);
}
```

### Using MD13S

```cpp
#include "md13s.h"

// Create MD13S instance
// MD13S(PWM_pin, DIR_pin, PWM_channel, frequency, resolution)
MD13S motor(25, 26, 0, 1000, 8);

void setup() {
    motor.begin();
}

void loop() {
    motor.forward(200);
    delay(2000);
    
    motor.stop();
    delay(1000);
    
    motor.reverse(200);
    delay(2000);
}
```

### Using Motor class (backward compatible with MD135)

```cpp
#include "motor.h"

// Motor is now typedef'd to MD135 (PWM + DIR control)
Motor motor(25, 26, 0);  // PWM, DIR, channel

void setup() {
    motor.begin();
}
```

## API Reference

### Common Methods (MD135 & MD13S)

- `begin()` - Initialize the motor driver
- `forward(int speed)` - Move forward at specified speed (0-255)
- `reverse(int speed)` - Move reverse at specified speed (0-255)
- `stop()` - Stop the motor
- `setSpeed(int speed)` - Change speed without changing direction
- `getSpeed()` - Get current speed value
- `isForward()` - Check if moving forward
- `isRunning()` - Check if motor is running
- `setDirection(bool forward)` - Change direction without changing speed
- `getMaxSpeed()` - Get maximum speed value for current resolution

## Wiring

### MD135 Wiring (3-wire control)
```
ESP32 Pin 25 → MD135 IN1
ESP32 Pin 26 → MD135 IN2
ESP32 Pin 27 → MD12-wire control)
```
ESP32 Pin 25 → MD135 PWM
ESP32 Pin 26 → MD135 DIRtrol)
```
ESP32 Pin 25 → MD13S PWM
ESP32 Pin 26 → MD13S DIR
ESP32 GND   → MD13S GND
```

## Pin Requirements

- **Direction pins**: Any digital GPIO
- **PWM pins**: Any PWM-capable pin on ESP32 (most GPIOs)
- **PWM channels**: 0-15 (ESP32 has 16 independent PWM channels)

## Notes

- Motor class is now a typedef to MD135 for backward compatibility
- For new projects, use MD135 or MD13S directly
- Default PWM frequency: 5000Hz (MD135), 1000Hz (MD13S)
- Default resolution: 8-bit (0-255 speed range)
