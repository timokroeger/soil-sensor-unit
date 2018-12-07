// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_RTU_PROTOCOL_H_
#define MODBUS_RTU_PROTOCOL_H_

#include "stdint.h"

#include "boost/sml.hpp"

#include "modbus/protocol_interface.h"
#include "modbus/rtu_protocol_internal.h"

namespace modbus {

namespace sml = boost::sml;

class RtuProtocol final : public ProtocolInterface {
 public:
  explicit RtuProtocol(SerialInterface& serial) : impl_(serial, buffer_, frame_available_) {}

  void Enable() { impl_.process_event(internal::Enable{}); }
  void Disable() { impl_.process_event(internal::Disable{}); }

  template <typename... Args>
  void Notify(Args&&... args) {
    impl_.process_event(std::forward<Args>(args)...);
  }

  virtual bool FrameAvailable() override { return frame_available_; };

  virtual FrameData ReadFrame() override {
    frame_available_ = false;
    return {buffer_.data(), buffer_.size()};
  };

  virtual void WriteFrame(FrameData fd) override {
    impl_.process_event(internal::TxStart{fd});
  };

 private:
  internal::RtuBuffer buffer_;
  bool frame_available_ = false;
  sml::sm<internal::RtuProtocol> impl_;
};

}  // namespace modbus

#endif  // MODBUS_RTU_PROTOCOL_H_
