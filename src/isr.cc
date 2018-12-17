// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "chip.h"

#include <cassert>

#include "globals.h"
#include "led.h"
#include "measure.h"

void SCT_Handler() { MeasureResetTrigger(); }

// MODBUS Timeouts
void MRT_Handler() {
  modbus_serial.TimerIsr();

  if (Chip_MRT_IntPending(LPC_MRT_CH2)) {
    Chip_MRT_IntClear(LPC_MRT_CH2);
    NVIC_SystemReset();
    assert(false);
  }

  if (Chip_MRT_IntPending(LPC_MRT_CH3)) {
    Chip_MRT_IntClear(LPC_MRT_CH3);
    LedTimeout();
  }
}

void UART0_Handler() {
  modbus_serial.UartIsr();
}
