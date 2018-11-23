// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "chip.h"

#include <assert.h>
#include <stdint.h>

#include "config.h"
#include "globals.h"
#include "led.h"
#include "measure.h"
#include "modbus.h"

// Increased when no stop bits were received.
static uint32_t uart_frame_error_counter = 0;

// Increased when there was a parity error.
static uint32_t uart_parity_error_counter = 0;

// Increased when only 2 of 3 samples of a UART bit reading were stable.
static uint32_t uart_noise_error_counter = 0;

void SCT_Handler() { MeasureResetTrigger(); }

// MODBUS Timeouts
void MRT_Handler() {
  if (Chip_MRT_IntPending(LPC_MRT_CH0)) {
    Chip_MRT_IntClear(LPC_MRT_CH0);
    modbus.Timeout();
  }

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
  uint32_t interrupt_status = Chip_UART_GetIntStatus(LPC_USART0);

  if (interrupt_status & UART_STAT_START) {
    Chip_MRT_SetInterval(LPC_MRT_CH0, 0 | MRT_INTVAL_LOAD);
    Chip_UART_ClearStatus(LPC_USART0, UART_STAT_START);
  }

  if (interrupt_status & UART_STAT_RXRDY) {
    bool parity_ok = true;

    // Start inter frame-delay timer.
    Chip_MRT_SetInterval(LPC_MRT_CH0,
                         ((CPU_FREQ / 1000000) * 1750) | MRT_INTVAL_LOAD);

    if (interrupt_status & UART_STAT_FRM_ERRINT) {
      uart_frame_error_counter++;
    }
    if (interrupt_status & UART_STAT_PAR_ERRINT) {
      uart_parity_error_counter++;
      parity_ok = false;
    }
    if (interrupt_status & UART_STAT_RXNOISEINT) {
      uart_noise_error_counter++;
    }

    uint8_t rxdata = static_cast<uint8_t>(Chip_UART_ReadByte(LPC_USART0));
    modbus.ByteReceived(rxdata, parity_ok);

    Chip_UART_ClearStatus(LPC_USART0,
                          UART_STAT_RXRDY | UART_STAT_FRM_ERRINT |
                              UART_STAT_PAR_ERRINT | UART_STAT_RXNOISEINT);
  }
}
