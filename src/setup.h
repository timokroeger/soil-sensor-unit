// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef SETUP_H_
#define SETUP_H_

#include <stdint.h>

// Enables LED output and sets ADC pin to analog mode.
void SetupGpio();

// Assigns peripherals to pins.
void SetupSwichMatrix();

// Calibrates the ADC and sets up a single measurement sequence which is
// triggered by the PWM timer.
void SetupAdc();

// Sets up a PWM output with 50% duty cycle used as excitation signal for the
// capacitive measurement. A second channel is used as ADC trigger.
void SetupPwm();

// Sets up 4 timer channels (clocked by FREQ_OSC) which generate an interrupt
// after timeout (see isr.c).
// Channel 0 and 1 are used for MODBUS timing.
// Channel 2 is used to delay system reset.
// Channel 3 is used to blink the LED.
void SetupTimers();

// Sets up UART0 with RTS pin as drive enable for the RS485 receiver.
void SetupUart(uint32_t baudrate);

// Configures all interrupts.
void SetupNVIC();

#endif  // SETUP_H_
