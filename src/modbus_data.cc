// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_data.h"

#include "bsp/bsp.h"
#include "version.h"

static uint16_t AverageMeasurement() {
  uint32_t acc = 0;

  for (int i = 0; i < (1 << 10); i++) {
    acc += BspMeasureRaw();
  }

  return acc >> 10;
}

void ModbusData::Complete() {
    meas_enabled_ = false;
    BspMeasurementDisable();
}

modbus::ExceptionCode ModbusData::ReadRegister(uint16_t address, uint16_t *data_out) {
  if (address == 0) {
    if (!meas_enabled_) {
      BspMeasurementEnable();
      meas_enabled_ = true;
    }

    *data_out = AverageMeasurement();
  } else if (address == 0x80) {
    *data_out = (VERSION_MAJOR << 8) | VERSION_MINOR;
  } else if (address == 0x100) {
    *data_out = reset_;
  } else {
    return modbus::ExceptionCode::kIllegalDataAddress;
  }

  return modbus::ExceptionCode::kOk;
}

modbus::ExceptionCode ModbusData::WriteRegister(uint16_t address, uint16_t data) {
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
