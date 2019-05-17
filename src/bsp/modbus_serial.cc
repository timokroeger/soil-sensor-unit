// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "bsp/modbus_serial.h"

void ModbusSerial::Init(uint32_t baudrate) {
  assert(usart_ != nullptr);
  assert(mrt_ch_ != nullptr);

  // Enable global UART clock. Divide clock down as much as possible.
  // HACKME: CLock precision could be improved by properly rounding.
  //         When rounding up Chip_UART_SetBaud() misbehaves though,
  //         because does not expect the input clock to be minimally
  //         slower than 16 times the baudrate.
  Chip_Clock_SetUARTClockDiv(Chip_Clock_GetMainClockRate() / (16 * baudrate) -
                             1);

  // Configure peripheral.
  Chip_UART_Init(usart_);
  Chip_UART_SetBaud(usart_, baudrate);
  Chip_UART_ConfigData(usart_, UART_CFG_DATALEN_8 | UART_CFG_PARITY_EVEN |
                                   UART_CFG_STOPLEN_1 | UART_CFG_OESEL |
                                   UART_CFG_OEPOL);

  // Enable receive and start interrupt. No interrupts for frame, parity or
  // noise errors are enabled because those are checked when reading a received
  // byte.
  Chip_UART_IntEnable(usart_, UART_INTEN_RXRDY | UART_INTEN_START);

  // Use a MRT Channel for MODBUS inter-frame timeout
  Chip_MRT_SetMode(mrt_ch_, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(mrt_ch_);  // Enable interrupt
}

void ModbusSerial::Enable() {
  assert(rtu_ != nullptr);

  // Assume that UART peripheral and the MRT timers were correctly initialized
  // in the startup code.
  Chip_UART_Enable(usart_);

  // Wait for a inter-frame timeout which then puts the stack in operational
  // (idle) state.
  Chip_MRT_SetInterval(
      mrt_ch_,
      ((Chip_Clock_GetSystemClockRate() / 1000000) * 1750) | MRT_INTVAL_LOAD);
}

void ModbusSerial::Disable() {
  Chip_MRT_SetInterval(mrt_ch_, 0 | MRT_INTVAL_LOAD);

  // Wait for the bus to be idle.
  uint32_t flags = (UART_STAT_RXIDLE | UART_STAT_TXIDLE);
  while ((Chip_UART_GetStatus(usart_) & flags) != flags) continue;

  Chip_UART_Disable(usart_);
}

void ModbusSerial::Send(const uint8_t* data, size_t length) {
  tx_active_ = true;
  tx_data_ = data;
  tx_data_end_ = data + length;
  Chip_UART_IntEnable(usart_, UART_INTEN_TXRDY);
}

void ModbusSerial::TimerIsr() {
  if (Chip_MRT_IntPending(mrt_ch_)) {
    Chip_MRT_IntClear(mrt_ch_);
    rtu_->BusIdle();
  }
}

void ModbusSerial::UartIsr() {
  uint32_t uart_ints = Chip_UART_GetIntStatus(usart_);

  if (uart_ints & UART_STAT_START) {
    Chip_MRT_SetInterval(mrt_ch_, 0 | MRT_INTVAL_LOAD);
    Chip_UART_ClearStatus(usart_, UART_STAT_START);
  }

  if (uart_ints & UART_STAT_RXRDY) {
    // Start inter frame-delay timer.
    Chip_MRT_SetInterval(
        mrt_ch_,
        ((Chip_Clock_GetSystemClockRate() / 1000000) * 1750) | MRT_INTVAL_LOAD);

    uint8_t rxdata = Chip_UART_ReadByte(usart_);
    rtu_->RxByte(rxdata, !(uart_ints & UART_STAT_PAR_ERRINT));

    Chip_UART_ClearStatus(usart_, UART_STAT_PAR_ERRINT);
  }

  if (uart_ints & UART_STAT_TXRDY) {
    assert(tx_data_ < tx_data_end_);
    Chip_UART_SendByte(usart_, *tx_data_);
    tx_data_++;
    if (tx_data_ >= tx_data_end_) {
      Chip_UART_IntDisable(usart_, UART_INTEN_TXRDY);
      Chip_UART_IntEnable(usart_, UART_INTEN_TXIDLE);
    }
  }

  if (uart_ints & UART_STAT_TXIDLE) {
    tx_active_ = false;
    rtu_->TxDone();
    Chip_UART_IntDisable(usart_, UART_STAT_TXIDLE);
  }
}
