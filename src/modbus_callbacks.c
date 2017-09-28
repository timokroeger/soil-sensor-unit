// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_callbacks.h"

#include "chip.h"

#define OSC_FREQ 12000000u

void ModbusSerialSend(uint8_t *data, int length)
{
  Chip_UART_SendBlocking(LPC_USART0, data, length);
}

void ModbusStartTimer(void)
{
  Chip_MRT_SetInterval(LPC_MRT_CH0, ((OSC_FREQ / 1000000) * 750) | MRT_INTVAL_LOAD);
  Chip_MRT_SetInterval(LPC_MRT_CH1, ((OSC_FREQ / 1000000) * 1750 ) | MRT_INTVAL_LOAD);
}

bool ModbusReadRegister(uint16_t address, uint16_t *data_out)
{
  (void)address;
  *data_out = 0u;
  return true;
}
