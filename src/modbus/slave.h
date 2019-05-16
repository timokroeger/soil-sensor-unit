// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_SLAVE_H_
#define MODBUS_SLAVE_H_

#include <assert.h>
#include <stdint.h>

#include "etl/bit_stream.h"

#include "modbus.h"
#include "modbus/data_interface.h"

namespace modbus {

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

// Parses modbus requests and passes data on to the used defined
// data interface.
class Slave {
 public:
  explicit Slave(DataInterface& data)
      : address_(-1),
        data_(data),
        response_(resp_buffer_.data(),
                  resp_buffer_.data() + resp_buffer_.capacity()) {}

  // Processes a request and creates a response.
  Buffer* Execute(const Buffer* req_buffer);

  // Valid range 1-247 inclusive.
  int address() const { return address_; }
  void set_address(int address) {
    assert(address > 0 && address <= 247);
    address_ = address;
  }

 private:
  ExceptionCode ReadInputRegister(etl::bit_stream& data);
  ExceptionCode WriteSingleRegister(etl::bit_stream& data);
  ExceptionCode WriteMultipleRegisters(etl::bit_stream& data);

  int address_;
  DataInterface& data_;
  Buffer resp_buffer_;
  etl::bit_stream response_;
};

}  // namespace modbus

#endif  // MODBUS_SLAVE_H_
