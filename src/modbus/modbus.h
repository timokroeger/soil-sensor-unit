// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_MODBUS_H_
#define MODBUS_MODBUS_H_

#include <assert.h>
#include <stdint.h>

#include "etl/vector.h"

#include "modbus/data_interface.h"
#include "modbus/protocol_interface.h"

namespace modbus {

// Parses modbus requests and passes data on to the used defined
// data interface.
class Modbus {
 public:
  Modbus(ProtocolInterface& protocol, DataInterface& data)
      : address_(-1), protocol_(protocol), data_(data) {}

  // Processes a request and sends a response.
  // Does nothing when no request is available.
  // Returns true when a request was processed, false otherwise.
  bool Execute();

  // Valid range 1-247 inclusive.
  int address() const { return address_; }
  void set_address(int address) {
    assert(address > 0 && address <= 247);
    address_ = address;
  }

 private:
  enum class FunctionCode {
    kReadInputRegister = 4,
    kWriteSingleRegister = 6,
    kWriteMultipleRegisters = 16,
  };

  enum class ExceptionCode {
    kInvalidFrame = -1,
    kOk = 0,
    kIllegalFunction,
    kIllegalDataAddress,
    kIllegalDataValue,
  };

  void ResponseAddByte(uint8_t byte);
  void ResponseAddWord(uint16_t word);

  ExceptionCode ReadInputRegister(FrameData data);

  int address_;
  ProtocolInterface& protocol_;
  DataInterface& data_;
  etl::vector<uint8_t, ProtocolInterface::kMaxFrameSize> resp_buffer_;
};

}  // namespace modbus

#endif  // MODBUS_MODBUS_H_
