// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_callbacks.h"

bool ModbusReadRegister(uint16_t address, uint16_t *data_out)
{
  (void)address;
  *data_out = 0u;
  return true;
}
