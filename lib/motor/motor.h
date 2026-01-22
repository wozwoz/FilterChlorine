#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

class Motor
{
private:
    uint8_t pin_in1;
    uint8_t pin_in2;
    uint8_t pin_enable;
    uint8_t pwm_channel;
    uint16_t pwm_frequency;
    uint8_t pwm_resolution;
    int current_speed;
    bool is_forward;

public:
    /**
     * Constructor for Motor H-Bridge driver (MD135, L298N, etc.)
     * @param in1 Direction control pin 1
     * @param in2 Direction control pin 2
     * @param enable PWM speed control pin (ENA/ENB)
     * @param channel PWM channel (0-15)
     * @param frequency PWM frequency in Hz (default: 5000)
     * @param resolution PWM resolution in bits (default: 8 for 0-255 range)
     */
    Motor(uint8_t in1, uint8_t in2, uint8_t enable, uint8_t channel = 0,
          uint16_t frequency = 5000, uint8_t resolution = 8);

    /**
     * Initialize motor pins and PWM
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
     * Stop the motor (coast to a stop)
     */
    void stop();

    /**
     * Brake the motor (active braking)
     */
    void brake();

    /**
     * Set motor speed without changing direction
     * @param speed Motor speed (0-255)
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
};

#endif // MOTOR_H
