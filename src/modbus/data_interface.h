// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_DATA_INTERFACE_H_
#define MODBUS_DATA_INTERFACE_H_

#include <stdint.h>

#include "modbus.h"

namespace modbus {

class DataInterface {
 public:
  virtual ~DataInterface() = default;

  // Reads the contents of a register at address and writes it to data_out.
  // Returns ExceptionCode::kOk on success or any other (positive) exception
  // code in case of failure.
  virtual ExceptionCode ReadRegister(uint16_t address, uint16_t *data_out) = 0;

  // Write data to a register.
  // Returns ExceptionCode::kOk on success or any other (positive) exception
  // code in case of failure.
  virtual ExceptionCode WriteRegister(uint16_t address, uint16_t data) = 0;
};

}  // namespace modbus

#endif  // MODBUS_DATA_INTERFACE_H_
