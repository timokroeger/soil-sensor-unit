// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_DATA_H_
#define MODBUS_DATA_H_

#include "modbus/data_interface.h"

class ModbusData final : public modbus::DataInterface {
 public:
  bool ReadRegister(uint16_t address, uint16_t *data_out) override;
  bool WriteRegister(uint16_t address, uint16_t data) override;
};

#endif  // MODBUS_DATA_H_
