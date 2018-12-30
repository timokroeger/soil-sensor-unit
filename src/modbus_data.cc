// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_data.h"
#include "measure.h"

static uint16_t AverageMeasurement() {
  uint32_t low = 0;
  uint32_t high = 0;

  for (int i = 0; i < (1 << 10); i++) {
    low += MeasureRaw(false);
    high += MeasureRaw(true);
  }

  return (high - low) >> 10;
}

bool ModbusData::ReadRegister(uint16_t address, uint16_t *data_out) {
  if (address == 0) {
    *data_out = AverageMeasurement();
    return true;
  }

  return false;
}

bool ModbusData::WriteRegister(uint16_t address, uint16_t data) {
  if (address == 0x1FFF) {
    if (data <= 0x8000) {
      return fw_update_.StartUpdate(data);
    } else if (data == 0xFFFF) {
      return fw_update_.ConfirmUpdate();
    } else {
      return false;
    }
  } else if (address >= 0x8000) {
    return fw_update_.AddImageData(address - 0x8000, data);
  }

  return false;
}
