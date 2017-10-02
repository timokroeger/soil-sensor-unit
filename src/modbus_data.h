// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_DATA_H_
#define MODBUS_DATA_H_

#include <stdint.h>

#include "modbus_data_interface.h"

class ModbusData : public ModbusDataInterface {
 public:
  bool ModbusReadRegister(uint16_t address, uint16_t *data_out) override;

  void set_raw_value(uint16_t v) { raw_value_ = v; }
  int raw_value() const { return raw_value_; }

 private:
  uint16_t raw_value_;
};

#endif  // MODBUS_DATA_H_