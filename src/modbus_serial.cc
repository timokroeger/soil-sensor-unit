// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_serial.h"

#include "chip.h"

#include "config.h"

void ModbusSerial::Enable() {
  assert(rtu_ != nullptr);

  // Assume that UART peripheral and the MRT timers were correctly initialized
  // in the startup code.
  Chip_UART_Enable(LPC_USART0);

  // Wait for a inter-frame timeout which then puts the stack in operational
  // (idle) state.
  Chip_MRT_SetInterval(LPC_MRT_CH0,
                       ((CPU_FREQ / 1000000) * 1750) | MRT_INTVAL_LOAD);
}

void ModbusSerial::Disable() {
  Chip_MRT_SetInterval(LPC_MRT_CH0, 0 | MRT_INTVAL_LOAD);

  // Wait for the bus to be idle.
  uint32_t flags = (UART_STAT_RXIDLE | UART_STAT_TXIDLE);
  while ((Chip_UART_GetStatus(LPC_USART0) & flags) != flags) continue;

  Chip_UART_Disable(LPC_USART0);
}

void ModbusSerial::Send(const uint8_t* data, size_t length) {
  tx_data_ = data;
  tx_data_end_ = data + length;
  Chip_UART_IntEnable(LPC_USART0, UART_INTEN_TXRDY);
}

void ModbusSerial::TimerIsr() {
  if (Chip_MRT_IntPending(LPC_MRT_CH0)) {
    Chip_MRT_IntClear(LPC_MRT_CH0);
    rtu_->Notify(BusIdle{});
  }
}

void ModbusSerial::UartIsr() {
  uint32_t uart_ints = Chip_UART_GetIntStatus(LPC_USART0);

  if (uart_ints & UART_STAT_START) {
    Chip_MRT_SetInterval(LPC_MRT_CH0, 0 | MRT_INTVAL_LOAD);
    Chip_UART_ClearStatus(LPC_USART0, UART_STAT_START);
  }

  if (uart_ints & UART_STAT_RXRDY) {
    // Start inter frame-delay timer.
    Chip_MRT_SetInterval(LPC_MRT_CH0,
                         ((CPU_FREQ / 1000000) * 1750) | MRT_INTVAL_LOAD);

    uint8_t rxdata = Chip_UART_ReadByte(LPC_USART0);
    rtu_->Notify(RxByte{rxdata, !(uart_ints & UART_STAT_PAR_ERRINT)});

    Chip_UART_ClearStatus(LPC_USART0, UART_STAT_PAR_ERRINT);
  }

  if (uart_ints & UART_STAT_TXRDY) {
    assert(tx_data_ < tx_data_end_);
    Chip_UART_SendByte(LPC_USART0, *tx_data_);
    tx_data_++;
    if (tx_data_ >= tx_data_end_) {
      Chip_UART_IntDisable(LPC_USART0, UART_INTEN_TXRDY);
      Chip_UART_IntEnable(LPC_USART0, UART_INTEN_TXIDLE);
    }
  }

  if (uart_ints & UART_STAT_TXIDLE) {
    rtu_->Notify(TxDone{});
    Chip_UART_IntDisable(LPC_USART0, UART_STAT_TXIDLE);
  }
}
