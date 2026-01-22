#include "motor.h"

Motor::Motor(uint8_t in1, uint8_t in2, uint8_t enable, uint8_t channel, 
             uint16_t frequency, uint8_t resolution) {
    pin_in1 = in1;
    pin_in2 = in2;
    pin_enable = enable;
    pwm_channel = channel;
    pwm_frequency = frequency;
    pwm_resolution = resolution;
    current_speed = 0;
    is_forward = true;
}

void Motor::begin() {
    // Configure direction control pins as outputs
    pinMode(pin_in1, OUTPUT);
    pinMode(pin_in2, OUTPUT);
    
    // Initialize pins to stopped state
    digitalWrite(pin_in1, LOW);
    digitalWrite(pin_in2, LOW);
    
    // Setup PWM channel for speed control
    ledcSetup(pwm_channel, pwm_frequency, pwm_resolution);
    ledcAttachPin(pin_enable, pwm_channel);
    ledcWrite(pwm_channel, 0);
    
    Serial.printf("Motor initialized: IN1=%d, IN2=%d, EN=%d, CH=%d\n", 
                  pin_in1, pin_in2, pin_enable, pwm_channel);
}

void Motor::forward(int speed) {
    // Constrain speed to valid range
    speed = constrain(speed, 0, (1 << pwm_resolution) - 1);
    
    // Set direction pins for forward
    digitalWrite(pin_in1, HIGH);
    digitalWrite(pin_in2, LOW);
    
    // Set speed via PWM
    ledcWrite(pwm_channel, speed);
    
    current_speed = speed;
    is_forward = true;
    
    Serial.printf("Motor forward: speed=%d\n", speed);
}

void Motor::reverse(int speed) {
    // Constrain speed to valid range
    speed = constrain(speed, 0, (1 << pwm_resolution) - 1);
    
    // Set direction pins for reverse
    digitalWrite(pin_in1, LOW);
    digitalWrite(pin_in2, HIGH);
    
    // Set speed via PWM
    ledcWrite(pwm_channel, speed);
    
    current_speed = speed;
    is_forward = false;
    
    Serial.printf("Motor reverse: speed=%d\n", speed);
}

void Motor::stop() {
    // Coast to a stop - both pins LOW
    digitalWrite(pin_in1, LOW);
    digitalWrite(pin_in2, LOW);
    ledcWrite(pwm_channel, 0);
    
    current_speed = 0;
    
    Serial.println("Motor stopped (coast)");
}

void Motor::brake() {
    // Active braking - both pins HIGH
    digitalWrite(pin_in1, HIGH);
    digitalWrite(pin_in2, HIGH);
    ledcWrite(pwm_channel, 0);
    
    current_speed = 0;
    
    Serial.println("Motor braking");
}

void Motor::setSpeed(int speed) {
    // Constrain speed to valid range
    speed = constrain(speed, 0, (1 << pwm_resolution) - 1);
    
    // Maintain current direction, just change speed
    if (current_speed > 0) {
        if (is_forward) {
            forward(speed);
        } else {
            reverse(speed);
        }
    } else {
        // If stopped, default to forward
        forward(speed);
    }
}

int Motor::getSpeed() {
    return current_speed;
}

bool Motor::isForward() {
    return is_forward;
}

bool Motor::isRunning() {
    return current_speed > 0;
}
