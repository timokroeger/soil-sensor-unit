// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "bsp/bsp.h"

// MODBUS Timeouts
void MRT_Handler() {
  modbus_serial.TimerIsr();
}

void UART0_Handler() {
  modbus_serial.UartIsr();
}
