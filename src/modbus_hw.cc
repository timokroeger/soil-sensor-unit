// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_hw.h"

#include "chip.h"

#include "common.h"

void ModbusHw::EnableHw() { Chip_UART_Enable(LPC_USART0); }

void ModbusHw::DisableHw() {
  Chip_MRT_SetInterval(LPC_MRT_CH0, 0 | MRT_INTVAL_LOAD);
  Chip_MRT_SetInterval(LPC_MRT_CH1, 0 | MRT_INTVAL_LOAD);

  // Wait for the bus to be idle.
  uint32_t flags = (UART_STAT_RXIDLE | UART_STAT_TXIDLE);
  while ((Chip_UART_GetStatus(LPC_USART0) & flags) != flags) continue;

  Chip_UART_Disable(LPC_USART0);
}

void ModbusHw::SerialSend(uint8_t* data, int length) {
  Chip_UART_SendBlocking(LPC_USART0, data, length);
}

void ModbusHw::StartTimer() {
  Chip_MRT_SetInterval(LPC_MRT_CH0,
                       ((OSC_FREQ / 1000000) * 750) | MRT_INTVAL_LOAD);
  Chip_MRT_SetInterval(LPC_MRT_CH1,
                       ((OSC_FREQ / 1000000) * 1750) | MRT_INTVAL_LOAD);
}
