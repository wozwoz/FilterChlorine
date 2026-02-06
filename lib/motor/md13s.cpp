#include "md13s.h"

MD13S::MD13S(uint8_t pwm, uint8_t dir, uint8_t channel, 
             uint16_t frequency, uint8_t resolution) {
    pin_pwm = pwm;
    pin_dir = dir;
    pwm_channel = channel;
    pwm_frequency = frequency;
    pwm_resolution = resolution;
    current_speed = 0;
    is_forward = true;
}

void MD13S::begin() {
    // Configure direction pin as output
    pinMode(pin_dir, OUTPUT);
    digitalWrite(pin_dir, LOW);  // Default to forward
    
    // Setup PWM channel for speed control
    ledcSetup(pwm_channel, pwm_frequency, pwm_resolution);
    ledcAttachPin(pin_pwm, pwm_channel);
    ledcWrite(pwm_channel, 0);  // Start with motor stopped
    
    Serial.printf("MD13S initialized: PWM=%d, DIR=%d, CH=%d, Freq=%dHz\n", 
                  pin_pwm, pin_dir, pwm_channel, pwm_frequency);
}

void MD13S::forward(int speed) {
    // Constrain speed to valid range based on PWM resolution
    speed = constrain(speed, 0, (1 << pwm_resolution) - 1);
    
    // Set direction to forward (LOW for MD13S)
    digitalWrite(pin_dir, LOW);
    
    // Set speed via PWM
    ledcWrite(pwm_channel, speed);
    
    current_speed = speed;
    is_forward = true;
    
    Serial.printf("MD13S forward: speed=%d\n", speed);
}

void MD13S::reverse(int speed) {
    // Constrain speed to valid range based on PWM resolution
    speed = constrain(speed, 0, (1 << pwm_resolution) - 1);
    
    // Set direction to reverse (HIGH for MD13S)
    digitalWrite(pin_dir, HIGH);
    
    // Set speed via PWM
    ledcWrite(pwm_channel, speed);
    
    current_speed = speed;
    is_forward = false;
    
    Serial.printf("MD13S reverse: speed=%d\n", speed);
}

void MD13S::stop() {
    // Stop motor by setting PWM to 0
    ledcWrite(pwm_channel, 0);
    
    current_speed = 0;
    
    Serial.println("MD13S stopped");
}

void MD13S::setSpeed(int speed) {
    // Constrain speed to valid range
    speed = constrain(speed, 0, (1 << pwm_resolution) - 1);
    
    // Maintain current direction, just change speed
    if (is_forward) {
        forward(speed);
    } else {
        reverse(speed);
    }
}

int MD13S::getSpeed() {
    return current_speed;
}

bool MD13S::isForward() {
    return is_forward;
}

bool MD13S::isRunning() {
    return current_speed > 0;
}

void MD13S::setDirection(bool forward) {
    int temp_speed = current_speed;
    
    if (forward) {
        this->forward(temp_speed);
    } else {
        this->reverse(temp_speed);
    }
}

int MD13S::getMaxSpeed() {
    return (1 << pwm_resolution) - 1;
}
