// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_SERIAL_INTERFACE_H_
#define MODBUS_SERIAL_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>

namespace modbus {

// Events that should the serial implementation should dispatch to the
// ProtocolImpl::Notify() method.
struct SerialInterfaceEvents {
  // A byte was received on the serial interface.
  struct RxByte {
    uint8_t byte;
    bool parity_ok;
  };

  // A MODBUS inter frame timeout occurred (= bus was idle for some time)
  //
  // The inter-frame delay (IFD) is the minimum time between two frames measured
  // between the end of the stop bit and the beginning of the start bit.
  // IFD = MAX(3.5 * 11 / baudrate, 1750us)
  struct BusIdle {};

  // A transfer started by the SerialInterface::Send() method has finished.
  struct TxDone {};
};

// Interface class for that must be implemented by the target platform code for
// the Modbus RTU or ASCII protocols.
class SerialInterface : public SerialInterfaceEvents {
 public:
  virtual ~SerialInterface() {}

  // Initializes the serial device and the timer required by the modbus stack.
  //
  // The user must configure the serial interface with the desired baudrate,
  // parity and stop bits. For RTU devices a timer must be setup to notify
  // inter-frame delays to the stack.
  // The BusIdle event should notfied right as soon as the bus is idle after
  // enabling the peripheral.
  virtual void Enable() = 0;

  // Disables the serial device and stops the timer.
  virtual void Disable() = 0;

  // Sends a modbus frame via the serial interface.
  // The data is valid and won’t be changed by the modbus stack until the
  // completion of the transmission is notified by a TxDone event.
  // This allows to implement DMA based transfer without the need to copy
  // message data to an additional buffer.
  virtual void Send(const uint8_t *data, size_t length) = 0;
};

}  // namespace modbus

#endif  // MODBUS_SERIAL_INTERFACE_H_
