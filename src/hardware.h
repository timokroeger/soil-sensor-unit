// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef HARDWARE_H_
#define HARDWARE_H_

#include <stdint.h>

// Sets up UART0 with RTS pin as drive enable for the RS485 receiver.
void HwSetupUart(uint32_t baudrate);

// Sets up the a timer channels 0 and 1 (clocked by FREQ_OSC) which generates
// an interrupt after timeout which is used for MODBUS timing.
void HwSetupTimers();

#endif  // HARDWARE_H_
