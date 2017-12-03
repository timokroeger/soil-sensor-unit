// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_DATA_H_
#define MODBUS_DATA_H_

#include <stdint.h>

#include "modbus_data_interface.h"

class ModbusData : public ModbusDataInterface {
 public:
  enum EventFlags {
    kResetDevice        = 0x01,
    kWriteConfiguration = 0x02,
  };

  ModbusData() : raw_value_(0), event_flags_(0) {}

  bool ReadRegister(uint16_t address, uint16_t *data_out) override;
  bool WriteRegister(uint16_t address, uint16_t data) override;

  void set_raw_value(uint16_t v) { raw_value_ = v; }
  uint16_t raw_value() const { return raw_value_; }

  uint32_t GetEvents();

 private:
  uint16_t raw_value_;
  uint32_t event_flags_;
};

#endif  // MODBUS_DATA_H_
