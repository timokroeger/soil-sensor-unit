// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MEASURE_H_
#define MEASURE_H_

#include <stdbool.h>
#include <stdint.h>

// Initializes hardware required for capacitance measurement (PWM and ADC).
void MeasureInit(void);

// Measures the capacitor voltage right before PWM switches states.
// The high parameter specifies which state of PWM to measure.
uint16_t MeasureRaw(bool high);

#endif  // MEASURE_H_
