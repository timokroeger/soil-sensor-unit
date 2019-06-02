// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_data.h"

#include "bsp/bsp.h"
#include "version.h"

void ModbusData::Complete() { measurement_available_ = false; }

modbus::ExceptionCode ModbusData::ReadRegister(uint16_t address,
                                               uint16_t *data_out) {
  if (address == 0) {
    if (!measurement_available_) {
      measumerent_ = BspMeasureRaw();
      measurement_available_ = true;
    }

    *data_out = measumerent_;
  } else if (address == 0x80) {
    *data_out = (VERSION_MAJOR << 8) | VERSION_MINOR;
  } else if (address == 0x100) {
    *data_out = reset_;
  } else {
    return modbus::ExceptionCode::kIllegalDataAddress;
  }

  return modbus::ExceptionCode::kOk;
}

modbus::ExceptionCode ModbusData::WriteRegister(uint16_t address,
                                                uint16_t data) {
  // Firmware update is mapped to the second half of the address range.
  if (address >= 0x8000) {
    return fw_update_.WriteRegister(address - 0x8000, data);
  } else if (address == 0x100) {
    reset_ = data;
  } else {
    return modbus::ExceptionCode::kIllegalDataAddress;
  }

  return modbus::ExceptionCode::kOk;
}
