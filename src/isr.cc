// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "chip.h"

#include <assert.h>
#include <stdint.h>

#include "config.h"
#include "globals.h"
#include "led.h"
#include "measure.h"

// Increased when no stop bits were received.
static uint32_t uart_frame_error_counter = 0;

// Increased when there was a parity error.
static uint32_t uart_parity_error_counter = 0;

// Increased when only 2 of 3 samples of a UART bit reading were stable.
static uint32_t uart_noise_error_counter = 0;

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
