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
  explicit RtuProtocol(SerialInterface& serial)
      : impl_(serial, rx_buffer_, frame_available_) {}

  void Enable() { impl_.process_event(internal::Enable{}); }
  void Disable() { impl_.process_event(internal::Disable{}); }

  template <typename... Args>
  void Notify(Args&&... args) {
    impl_.process_event(std::forward<Args>(args)...);
  }

  Buffer *ReadFrame() {
    if (!frame_available_) {
      return nullptr;
    }

    frame_available_ = false;
    return &rx_buffer_;
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
