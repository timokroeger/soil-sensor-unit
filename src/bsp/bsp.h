// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef BSP_BSP_H_
#define BSP_BSP_H_

#include "bsp/modbus_serial.h"

extern ModbusSerial modbus_serial;

void BspSetup();
void BspReset();

#endif  // BSP_BSP_H_
