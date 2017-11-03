// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_data.h"

#include "config_storage.h"

#define CONFIG_OFFSET 128u

bool ModbusData::ReadRegister(uint16_t address, uint16_t *data_out) {
  if (address == 0) {
    *data_out = raw_value_;
  } else if (address >= CONFIG_OFFSET &&
             address < ConfigStorage::kMaxConfigIndex + CONFIG_OFFSET) {
    *data_out = ConfigStorage::Instance().Get(
        static_cast<ConfigStorage::ConfigIndex>(address - CONFIG_OFFSET));
  } else {
    return false;
  }

  return true;
}

bool ModbusData::WriteRegister(uint16_t address, uint16_t data) {
  if (address >= CONFIG_OFFSET &&
      address < ConfigStorage::kMaxConfigIndex + CONFIG_OFFSET) {
    ConfigStorage cs = ConfigStorage::Instance();
    cs.Set(static_cast<ConfigStorage::ConfigIndex>(address - CONFIG_OFFSET),
           data);
    cs.WriteConfigToFlash();
    return true;
  }

  return false;
}
