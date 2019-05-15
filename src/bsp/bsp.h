// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef BSP_BSP_H_
#define BSP_BSP_H_

#include <cstdint>

#include "bsp/modbus_serial.h"

extern ModbusSerial modbus_serial;

void BspSetup();

void BspReset();

// Measures the differential sampling capacitor voltage.
uint16_t BspMeasureRaw();

#endif  // BSP_BSP_H_
