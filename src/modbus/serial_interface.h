// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_SERIAL_INTERFACE_H_
#define MODBUS_SERIAL_INTERFACE_H_

#include <stddef.h>

namespace modbus {

// Interface class for that must be implemented by the target platform code for
// the Modbus RTU or ASCII protocols.
class SerialInterface {
 public:
  virtual ~SerialInterface() {}

  // Initializes the serial device and the timer required by the modbus stack.
  //
  // The user must configure the serial interface with the desired baudrate,
  // parity and stop bits. For RTU devices a timer must be setup to signal
  // inter-frame delays to the stack.
  virtual void Enable() = 0;

  // Disables the serial device and stops the timer.
  virtual void Disable() = 0;

  // Sends a modbus frame via the serial interface.
  virtual void Send(const uint8_t *data, size_t length) = 0;
};

}  // namespace modbus

#endif  // MODBUS_SERIAL_INTERFACE_H_
