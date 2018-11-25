// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_HW_INTERFACE_H_
#define MODBUS_HW_INTERFACE_H_

#include <stddef.h>

class ModbusHwInterface {
 public:
  virtual ~ModbusHwInterface() {}

  // Initializes the serial device and the timer required by the modbus stack.
  //
  // The user must configure the serial interface with the desired baudrate,
  // parity and stop bits.
  // A timer must be setup to signal inter-frame delays to the stack with the
  // Timeout() method.
  virtual void EnableHw() = 0;

  // Disables the serial device and stops the timer.
  // Make sure to stop an disable timers first so that a timer interrupt does
  // not cause any further processing inside the MODBUS stack.
  virtual void DisableHw() = 0;

  // Sends a modbus response via the serial interface.
  virtual void SerialSend(const uint8_t *data, size_t length) = 0;
};

#endif  // MODBUS_HW_INTERFACE_H_
