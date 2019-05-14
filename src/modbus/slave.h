// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_SLAVE_H_
#define MODBUS_SLAVE_H_

#include <assert.h>
#include <stdint.h>

#include "etl/array.h"
#include "etl/array_view.h"
#include "etl/bit_stream.h"
#include "etl/optional.h"

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
        response_(resp_buffer_.begin(), resp_buffer_.end()) {}

  // Processes a request and creates a response.
  // Returns some array_view of the response data.
  etl::optional<etl::const_array_view<uint8_t>> Execute(
      etl::const_array_view<uint8_t> fd);

  // Valid range 1-247 inclusive.
  int address() const { return address_; }
  void set_address(int address) {
    assert(address > 0 && address <= 247);
    address_ = address;
  }

 private:
  static constexpr size_t kMaxFrameSize = 256;

  ExceptionCode ReadInputRegister(etl::bit_stream& data);
  ExceptionCode WriteSingleRegister(etl::bit_stream& data);
  ExceptionCode WriteMultipleRegisters(etl::bit_stream& data);

  int address_;
  DataInterface& data_;
  etl::array<uint8_t, kMaxFrameSize> resp_buffer_;
  etl::bit_stream response_;
};

}  // namespace modbus

#endif  // MODBUS_SLAVE_H_
