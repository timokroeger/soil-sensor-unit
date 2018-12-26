// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "chip.h"

#include <cassert>

#include "globals.h"
#include "measure.h"

void SCT_Handler() { MeasureResetTrigger(); }

// MODBUS Timeouts
void MRT_Handler() {
  modbus_serial.TimerIsr();
}

void UART0_Handler() {
  modbus_serial.UartIsr();
}
