// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_hw.h"

#include "chip.h"

#include "common.h"
#include "config_storage.h"
#include "expect.h"
#include "modbus.h"

Modbus* ModbusHw::modbus_ = nullptr;

ModbusHw::ModbusHw() {
  SetupUart();
  SetupTimers();
}

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

void ModbusHw::SetupUart() {
  // Enable global UART clock.
  Chip_Clock_SetUARTClockDiv(1);

  Chip_UART_Init(LPC_USART0);

  uint32_t baudrate = ConfigStorage::Instance().Get(ConfigStorage::kBaudrate);
  Chip_UART_SetBaud(LPC_USART0, baudrate);
  Chip_UART_ConfigData(LPC_USART0, UART_CFG_DATALEN_8 | UART_CFG_PARITY_EVEN |
                                       UART_CFG_STOPLEN_1 | UART_CFG_OESEL |
                                       UART_CFG_OEPOL);

  // Enable receive and start interrupt. No interrupts for frame, parity or
  // noise errors are enabled because those are checked when reading a received
  // byte.
  Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY | UART_INTEN_START);
}

void ModbusHw::SetupTimers() {
  Chip_MRT_Init();

  // Channel 0: MODBUS inter-character timeout
  Chip_MRT_SetMode(LPC_MRT_CH0, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH0);

  // Channel 1: MODBUS inter-frame timeout
  Chip_MRT_SetMode(LPC_MRT_CH1, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH1);
}

