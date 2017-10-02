// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_HW_INTERFACE_H_
#define MODBUS_HW_INTERFACE_H_

#include <stdint.h>

class ModbusHwInterface {
 public:
  // Initalises the serial interface and the timers required by the modbus
  // stack.
  //
  // The user must configure the serial interface with the desired baudrate,
  // parity and stop bits. As inter-character and inter-frame timeout values
  // depend on baudrate it is also up to the user to decide on those.
  virtual void SerialEnable() = 0;

  // Sends a modbus response via serial interface.
  virtual void SerialSend(uint8_t *data, int length) = 0;

  // This function shall start a one-shot timer which calls ModbusTimeout()
  // first kModbusTimeoutInterCharacterDelay after typically 750us and then
  // kModbusTimeoutInterFrameDelay after typically 1750us.
  virtual void StartTimer() = 0;
};

#endif  // MODBUS_HW_INTERFACE_H_
