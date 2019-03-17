// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "globals.h"

void USART2_IRQHandler() {
  modbus_serial.Isr();
}
