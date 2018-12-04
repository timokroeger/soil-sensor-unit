// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_DATA_INTERFACE_H_
#define MODBUS_DATA_INTERFACE_H_

namespace modbus {

#include <stdint.h>

class DataInterface {
 public:
  virtual ~DataInterface() {}

  // Reads the contents of a register at address and writes it to data_out.
  // Returns true on success or false when the register is not available.
  virtual bool ReadRegister(uint16_t address, uint16_t *data_out) = 0;

  // Write data to a register.
  // Returns true on success or false when the register is not available.
  virtual bool WriteRegister(uint16_t address, uint16_t data) = 0;
};

}  // namespace modbus

#endif  // MODBUS_DATA_INTERFACE_H_
