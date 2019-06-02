// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef BSP_BSP_H_
#define BSP_BSP_H_

#include <cstdint>

#include "bsp/bootloader.h"
#include "bsp/modbus_serial.h"

struct RawMeasurement {
  uint16_t low;
  uint16_t high;
  uint16_t diodes;
};

extern Bootloader bootloader;
extern ModbusSerial modbus_serial;

void BspSetup();
void BspSetupPins();

void BspSleep();

void BspReset();

RawMeasurement BspMeasureRaw();

class BspInterruptFree {
 public:
  BspInterruptFree();
  ~BspInterruptFree();
};

#endif  // BSP_BSP_H_
