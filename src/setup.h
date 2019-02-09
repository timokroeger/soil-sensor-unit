// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef SETUP_H_
#define SETUP_H_

#include <cstdint>

// Frequency of the main and system clock.
#define MAIN_FREQ 60000000
#define SYSTEM_FREQ 30000000

// Configures system clock to 30MHz with the PLL fed by the internal oscillator.
void SetupClock();

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
// Channel 0 is used for MODBUS timing.
void SetupTimers();

// Sets up UART0 with RTS pin as drive enable for the RS485 receiver.
void SetupUart(uint32_t baudrate);

// Configures all interrupts.
void SetupNVIC();

#endif  // SETUP_H_
