// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_data.h"

#include "chip.h"

#include "config_storage.h"
#include "expect.h"

#define RESET_OFFSET 256u
#define CONFIG_OFFSET 257u

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
    event_flags_ |= kWriteConfiguration;
  } else if (address == RESET_OFFSET) {
    if (data != 0) {
      event_flags_ |= kResetDevice;
    }
  } else {
    return false;
  }

  return true;
}

uint32_t ModbusData::GetEvents() {
  uint32_t ret = event_flags_;
  event_flags_ = 0;
  return ret;
}
