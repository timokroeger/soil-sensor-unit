// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MEASURE_H_
#define MEASURE_H_

#include <cstdint>

// Starts the PWM and ADC peripheral.
void MeasureStart();

// Measures the capacitor voltage right before PWM switches states.
// The high parameter specifies which state of PWM to measure.
uint16_t MeasureRaw(bool high);

// Clears ADC trigger and stops any further conversions.
// Must be called from the timer interrupt that triggers the ADC.
void MeasureResetTrigger();

#endif  // MEASURE_H_
