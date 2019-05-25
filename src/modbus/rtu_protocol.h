// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_RTU_PROTOCOL_H_
#define MODBUS_RTU_PROTOCOL_H_

#include <stdint.h>

#include "boost/sml.hpp"

#include "modbus.h"
#include "modbus/rtu_protocol_internal.h"

namespace modbus {

namespace sml = boost::sml;

class RtuProtocol {
 public:
  explicit RtuProtocol(SerialInterface &serial)
      : impl_(serial, rx_buffer_, frame_available_) {}

  // A MODBUS inter frame timeout occurred (= bus was idle for some time)
  //
  // The inter-frame delay (IFD) is the minimum time between two frames measured
  // between the end of the stop bit and the beginning of the start bit.
  // IFD = MAX(3.5 * 11 / baudrate, 1750us)
  void BusIdle() {
    impl_.process_event(internal::BusIdle{});
  }

  // A byte was received on the serial interface.
  void RxByte(uint8_t byte, bool parity_ok) {
    impl_.process_event(internal::RxByte{byte, parity_ok});
  }

  // A transfer started by the SerialInterface::Send() method has finished.
  void TxDone() {
    impl_.process_event(internal::TxDone{});
  }

  UniqueBuffer ReadFrame() {
    if (!frame_available_) {
      return nullptr;
    }

    frame_available_ = false;
    return UniqueBuffer(&rx_buffer_);
  };

  void WriteFrame(Buffer *buffer) {
    impl_.process_event(internal::TxStart{buffer});
  };

 private:
  Buffer rx_buffer_;
  bool frame_available_ = false;
  sml::sm<internal::RtuProtocol> impl_;
};

}  // namespace modbus

#endif  // MODBUS_RTU_PROTOCOL_H_
