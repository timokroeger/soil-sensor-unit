// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_data.h"

bool ModbusData::ReadRegister(uint16_t address, uint16_t *data_out) {
  (void)address;
  *data_out = raw_value_;
  return true;
}
