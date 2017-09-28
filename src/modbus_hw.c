// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_hw.h"

#include "chip.h"

#include "expect.h"
#include "modbus.h"

#define UART_BAUDRATE 19200u
#define OSC_FREQ 12000000u

// Increased when no stop bits were received.
static uint32_t uart_frame_error_counter = 0;

// No parity bit is used in the GaMoSy Communication Protocol so this should
// always stay at 0.
static uint32_t uart_parity_error_counter = 0;

// Increased when only 2 of 3 samples of a UART bit reading were stable.
static uint32_t uart_noise_error_counter = 0;

// Setup UART0 with RTS pin as drive enable for the RS485 receiver.
static void SetupUart(void) {
  // Enable global UART clock.
  Chip_Clock_SetUARTClockDiv(1);

  Chip_UART_Init(LPC_USART0);
  Chip_UART_SetBaud(LPC_USART0, UART_BAUDRATE);
  Chip_UART_ConfigData(LPC_USART0,
                       UART_CFG_ENABLE | UART_CFG_DATALEN_8 |
                           UART_CFG_PARITY_EVEN | UART_CFG_STOPLEN_1 |
                           UART_CFG_OESEL | UART_CFG_OEPOL);

  // Enable receive and overrun interrupt. No interrupts for frame, parity or
  // noise errors are enabled because those are checked when reading a received
  // byte.
  Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY | UART_INTEN_OVERRUN);
}

static void SetupTimers(void) {
  Chip_MRT_Init();

  // Channel 0: MODBUS inter-character timeout
  Chip_MRT_SetMode(LPC_MRT_CH0, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH0);

  // Channel 1: MODBUS inter-frame timeout
  Chip_MRT_SetMode(LPC_MRT_CH1, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH1);
}

void ModbusInitHw(void) {
  SetupUart();
  SetupTimers();
}

void ModbusSerialSend(uint8_t *data, int length) {
  Chip_UART_SendBlocking(LPC_USART0, data, length);
}

void ModbusStartTimer(void) {
  Chip_MRT_SetInterval(LPC_MRT_CH0,
                       ((OSC_FREQ / 1000000) * 750) | MRT_INTVAL_LOAD);
  Chip_MRT_SetInterval(LPC_MRT_CH1,
                       ((OSC_FREQ / 1000000) * 1750) | MRT_INTVAL_LOAD);
}

void UART0_IRQHandler(void) {
  uint32_t interrupt_status = Chip_UART_GetIntStatus(LPC_USART0);
  Expect((interrupt_status & (UART_STAT_RXRDY | UART_STAT_OVERRUNINT)) ==
         UART_STAT_RXRDY);

  if (interrupt_status & UART_STAT_FRM_ERRINT) {
    uart_frame_error_counter++;
  }
  if (interrupt_status & UART_STAT_PAR_ERRINT) {
    uart_parity_error_counter++;
  }
  if (interrupt_status & UART_STAT_RXNOISEINT) {
    uart_noise_error_counter++;
  }

  uint8_t rxdata = (uint8_t)Chip_UART_ReadByte(LPC_USART0);
  ModbusByteReceived(rxdata);

  Chip_UART_ClearStatus(LPC_USART0,
                        UART_STAT_RXRDY | UART_STAT_FRM_ERRINT |
                            UART_STAT_PAR_ERRINT | UART_STAT_RXNOISEINT);
}

void MRT_IRQHandler(void) {
  if (Chip_MRT_IntPending(LPC_MRT_CH0)) {
    Chip_MRT_IntClear(LPC_MRT_CH0);
    ModbusTimeout(kModbusTimeoutInterCharacterDelay);
  }

  if (Chip_MRT_IntPending(LPC_MRT_CH1)) {
    Chip_MRT_IntClear(LPC_MRT_CH1);
    ModbusTimeout(kModbusTimeoutInterFrameDelay);
  }
}
