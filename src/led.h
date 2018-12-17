// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef LED_H_
#define LED_H_

#define LED_OFF_MS 100
#define LED_ON_MS 100

// Blinks the LED.
// LED is default on. When blinking one time it is disabled for LED_OFF_US
// microseconds. When blinking multiple times it is enabled for LED_ON_US
// before it is turned off again.
void LedBlink(int times);

// Must be called by the timer ISR.
void LedTimeout();

#endif  // LED_H_
