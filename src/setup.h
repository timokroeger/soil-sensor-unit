// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef SETUP_H_
#define SETUP_H_

#include <stdint.h>

// Assigns peripherals to pins.
void SetupSwichMatrix();

// Initializes the chip to use an external 12MHz crystal as system clock without
// PLL.
void SetupMainClockCrystal();

// Calibrates the ADC and sets up a single measurement sequence which is
// triggered by the PWM timer.
void SetupAdc();

// Sets up a PWM output with 50% duty cycle used as excitation signal for the
// capacitive measurement. A second channel is used as ADC trigger.
void SetupPwm();

// Sets up the a timer channels 0 and 1 (clocked by FREQ_OSC) which generates
// an interrupt after timeout which is used for MODBUS timing.
void SetupTimers();

// Sets up UART0 with RTS pin as drive enable for the RS485 receiver.
void SetupUart(uint32_t baudrate);

// Configures all interrupts.
void SetupNVIC();

#endif  // SETUP_H_
