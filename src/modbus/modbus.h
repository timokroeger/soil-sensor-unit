// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_MODBUS_H_
#define MODBUS_MODBUS_H_

#include <assert.h>
#include <stdint.h>

#include "etl/array.h"
#include "etl/bit_stream.h"

#include "modbus/data_interface.h"
#include "modbus/protocol_interface.h"

namespace modbus {

// Parses modbus requests and passes data on to the used defined
// data interface.
class Modbus {
 public:
  Modbus(ProtocolInterface& protocol, DataInterface& data)
      : address_(-1),
        protocol_(protocol),
        data_(data),
        response_(resp_buffer_.begin(), resp_buffer_.end()) {}

  // Processes a request and sends a response.
  // Does nothing when no request is available.
  // Returns true when a request was processed successfully, false otherwise.
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

  ExceptionCode ReadInputRegister(etl::bit_stream& data);
  ExceptionCode WriteSingleRegister(etl::bit_stream& data);
  ExceptionCode WriteMultipleRegisters(etl::bit_stream& data);

  int address_;
  ProtocolInterface& protocol_;
  DataInterface& data_;
  etl::array<uint8_t, ProtocolInterface::kMaxFrameSize> resp_buffer_;
  etl::bit_stream response_;
};

}  // namespace modbus

#endif  // MODBUS_MODBUS_H_
