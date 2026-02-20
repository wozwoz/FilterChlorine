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
}

void MD13S::forward(int speed) {
    // Constrain speed to valid range based on PWM resolution
    speed = constrain(speed, 0, (1 << pwm_resolution) - 1);
    
    // If changing direction, stop motor and wait for it to fully stop
    if (!is_forward && current_speed > 0) {
        ledcWrite(pwm_channel, 0);
        current_speed = 0;
        delay(500);  // Wait for motor to completely stop spinning
    }
    
    // Set direction to forward (LOW for MD13S)
    digitalWrite(pin_dir, LOW);
    delay(50);
    
    // Very gradual ramp-up to prevent power surge
    // Start at just 10% speed and increase slowly
    if (speed > 0) {
        int startSpeed = max(10, speed / 10);  // Start at 10% of target
        ledcWrite(pwm_channel, startSpeed);
        delay(100);  // Let it stabilize
        
        // Ramp up in small steps
        for (int i = startSpeed; i < speed; i += 10) {
            ledcWrite(pwm_channel, i);
            delay(20);  // Slower ramp
        }
    }
    ledcWrite(pwm_channel, speed);
    
    current_speed = speed;
    is_forward = true;
}

void MD13S::reverse(int speed) {
    // Constrain speed to valid range based on PWM resolution
    speed = constrain(speed, 0, (1 << pwm_resolution) - 1);
    
    // If changing direction, stop motor and wait for it to fully stop
    if (is_forward && current_speed > 0) {
        ledcWrite(pwm_channel, 0);
        current_speed = 0;
        delay(500);  // Wait for motor to completely stop spinning
    }
    
    // Set direction to reverse (HIGH for MD13S)
    digitalWrite(pin_dir, HIGH);
    delay(50);
    
    // Very gradual ramp-up to prevent power surge
    // Start at just 10% speed and increase slowly
    if (speed > 0) {
        int startSpeed = max(10, speed / 10);  // Start at 10% of target
        ledcWrite(pwm_channel, startSpeed);
        delay(100);  // Let it stabilize
        
        // Ramp up in small steps
        for (int i = startSpeed; i < speed; i += 10) {
            ledcWrite(pwm_channel, i);
            delay(20);  // Slower ramp
        }
    }
    ledcWrite(pwm_channel, speed);
    
    current_speed = speed;
    is_forward = false;
}

void MD13S::stop() {
    // Stop motor by setting PWM to 0
    ledcWrite(pwm_channel, 0);
    
    current_speed = 0;
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
