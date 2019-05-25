// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_SLAVE_H_
#define MODBUS_SLAVE_H_

#include <assert.h>
#include <stdint.h>

#include "etl/bit_stream.h"

#include "modbus.h"
#include "modbus/data_interface.h"

namespace modbus {

// Parses modbus requests and passes data on to the used defined
// data interface.
class Slave {
 public:
  explicit Slave(DataInterface& data)
      : address_(-1),
        data_(data) {}

  // Processes a request and creates a response.
  bool Execute(const Buffer* req_buffer, Buffer *resp_buffer);

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
  etl::bit_stream response_;
};

}  // namespace modbus

#endif  // MODBUS_SLAVE_H_
