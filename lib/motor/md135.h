#ifndef MD135_H
#define MD135_H

#include <Arduino.h>

/**
 * MD135 Motor Driver Class
 * 
 * The MD135 is a high-power DC motor driver
 * Features:
 * - Single DC motor control
 * - PWM speed control
 * - Direction control
 * - High current capacity
 * - 2-wire control interface (PWM + DIR)
 */
class MD135 {
private:
    uint8_t pin_pwm;        // PWM pin for speed control
    uint8_t pin_dir;        // Direction pin
    uint8_t pwm_channel;    // ESP32 PWM channel
    uint16_t pwm_frequency; // PWM frequency in Hz
    uint8_t pwm_resolution; // PWM resolution in bits
    int current_speed;      // Current speed value
    bool is_forward;        // Current direction

public:
    /**
     * Constructor for MD135 motor driver
     * @param pwm PWM speed control pin
     * @param dir Direction control pin
     * @param channel PWM channel (0-15 on ESP32)
     * @param frequency PWM frequency in Hz (default: 5000)
     * @param resolution PWM resolution in bits (default: 8 for 0-255 range)
     */
    MD135(uint8_t pwm, uint8_t dir, uint8_t channel = 0,
          uint16_t frequency = 5000, uint8_t resolution = 8);

    /**
     * Initialize MD135 pins and PWM
     */
    void begin();

    /**
     * Set motor to move forward at specified speed
     * @param speed Motor speed (0-255 for 8-bit resolution)
     */
    void forward(int speed);

    /**
     * Set motor to move in reverse at specified speed
     * @param speed Motor speed (0-255 for 8-bit resolution)
     */
    void reverse(int speed);

    /**
     * Stop the motor
     */
    void stop();

    /**
     * Set motor speed without changing direction
     * @param speed Motor speed (0-255 for 8-bit resolution)
     */
    void setSpeed(int speed);

    /**
     * Get current motor speed
     * @return Current speed value
     */
    int getSpeed();

    /**
     * Check if motor is moving forward
     * @return true if forward, false if reverse
     */
    bool isForward();

    /**
     * Check if motor is currently running
     * @return true if motor speed > 0
     */
    bool isRunning();

    /**
     * Set motor direction without changing speed
     * @param forward true for forward, false for reverse
     */
    void setDirection(bool forward);

    /**
     * Get the maximum speed value based on PWM resolution
     * @return Maximum speed value
     */
    int getMaxSpeed();
};

#endif // MD135_H
