// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_SERIAL_INTERFACE_H_
#define MODBUS_SERIAL_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>

namespace modbus {

// Interface class for that must be implemented by the target platform code for
// the Modbus RTU or ASCII protocols.
//
// The user must configure the serial interface with the desired baudrate,
// parity and stop bits. For RTU devices a timer must be setup to notify
// inter-frame delays to the protocol stack with the BusIdle() method.
// The BusIdle event should notfied right as soon as the bus is idle after
// enabling the peripheral.
class SerialInterface {
 public:
  virtual ~SerialInterface() {}

  // Sends a modbus frame via the serial interface.
  // The data is valid and won’t be changed by the modbus stack until the
  // completion of the transmission is notified with the TxDone() method.
  // This allows to implement DMA based transfer without the need to copy
  // message data to an additional buffer.
  virtual void Send(const uint8_t *data, size_t length) = 0;
};

}  // namespace modbus

#endif  // MODBUS_SERIAL_INTERFACE_H_
