// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "hardware.h"

#include "chip.h"

#include "common.h"

void HwSetupUart(uint32_t baudrate) {
  // Enable global UART clock.
  Chip_Clock_SetUARTClockDiv(1);

  // Configure peripheral.
  Chip_UART_Init(LPC_USART0);
  Chip_UART_SetBaud(LPC_USART0, baudrate);
  Chip_UART_ConfigData(LPC_USART0, UART_CFG_DATALEN_8 | UART_CFG_PARITY_EVEN |
                                       UART_CFG_STOPLEN_1 | UART_CFG_OESEL |
                                       UART_CFG_OEPOL);

  // Enable receive and start interrupt. No interrupts for frame, parity or
  // noise errors are enabled because those are checked when reading a received
  // byte.
  Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY | UART_INTEN_START);
}

void HwSetupTimers() {
  Chip_MRT_Init();

  // Channel 0: MODBUS inter-character timeout
  Chip_MRT_SetMode(LPC_MRT_CH0, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH0);

  // Channel 1: MODBUS inter-frame timeout
  Chip_MRT_SetMode(LPC_MRT_CH1, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH1);
}
