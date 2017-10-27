// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_data.h"

#include "config_storage.h"

bool ModbusData::ReadRegister(uint16_t address, uint16_t *data_out) {
  (void)address;
  *data_out = raw_value_;
  return true;
}

bool ModbusData::WriteRegister(uint16_t address, uint16_t data) {
  if (address < ConfigStorage::kMaxConfigIndex) {
    ConfigStorage cs = ConfigStorage::Instance();
    cs.Set(static_cast<ConfigStorage::ConfigIndex>(address), data);
    cs.WriteConfigToFlash();
    return true;
  }

  return false;
}
