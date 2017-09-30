// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_CALLBACKS_H_
#define MODBUS_CALLBACKS_H_

#include <stdint.h>

// Reads the contents of a register at address and writes it to data_out.
// Returns true on success or false when the register is not available.
bool ModbusReadRegister(uint16_t address, uint16_t *data_out);

#endif  // MODBUS_CALLBACKS_H_
