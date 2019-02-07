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
  // Firmware update is mapped to the second half of the address range.
  if (address >= 0x8000) {
    fw_update_.WriteRegister(address - 0x8000, data);
  }

  return false;
}
