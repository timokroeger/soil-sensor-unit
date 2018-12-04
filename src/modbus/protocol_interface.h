// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_PROTOCOL_INTERFACE_H_
#define MODBUS_PROTOCOL_INTERFACE_H_

#include <stdint.h>

#include "etl/array_view.h"

namespace modbus {

using FrameData = etl::const_array_view<uint8_t>;

// Interface class for underlying Modbus protocol.
// Possible specializations are RTU, ASCII or TCP.
class ProtocolInterface {
 public:
  static constexpr int kMaxFrameSize = 256;

  virtual ~ProtocolInterface(){};

  // Returns true when a valid frame was received.
  virtual bool FrameAvailable() = 0;

  // Read the last received frame.
  // Behavior is undefined when no frame is available. 
  virtual FrameData ReadFrame() = 0;

  // Returns true when the frame was written successfully.
  virtual bool WriteFrame(FrameData fd) = 0;
};

}  // namespace modbus

#endif  // MODBUS_PROTOCOL_INTERFACE_H_
