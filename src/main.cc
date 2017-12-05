// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include <stdint.h>

#include "chip.h"

#include "common.h"
#include "config_storage.h"
#include "expect.h"
#include "globals.h"
#include "hardware.h"
#include "measure.h"
#include "modbus.h"
#include "modbus_data.h"

#define ISR_PRIO_UART 1
#define ISR_PRIO_TIMER 1

// Must have highest priority so that the ADC trigger can be reset in time.
#define ISR_PRIO_SCT 0

const uint32_t OscRateIn = OSC_FREQ;
const uint32_t ExtRateIn = 0;  // External clock input not used.

// Delays execution by a few ticks.
static inline void WaitTicks(uint32_t ticks) {
  uint32_t num_iters = ticks / 3;  // ASM code takes 3 ticks for each iteration.
  __asm__ volatile("0: SUB %[i],#1; BNE 0b;" : [i] "+r"(num_iters));
}

static void InitSwichMatrix() {
  // Enable switch matrix.
  Chip_SWM_Init();
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);

  // Select XTAL functionality for PIO0_8 and PIO0_9.
  Chip_SWM_EnableFixedPin(SWM_FIXED_XTALIN);
  Chip_SWM_EnableFixedPin(SWM_FIXED_XTALOUT);

  // Disable Pull-Ups on crystal pins.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO8, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO9, PIN_MODE_INACTIVE);

  // UART0
  Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 4);
  Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, 15);
  Chip_SWM_MovablePinAssign(SWM_U0_RTS_O, 1);

  // PWM output
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT0_O, 0);   // FREQ_LO
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT1_O, 14);  // FREQ_HI

  // ADC input
  Chip_SWM_EnableFixedPin(SWM_FIXED_ADC3);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO23, PIN_MODE_INACTIVE);

  // Switch matrix clock is not needed anymore after configuration.
  Chip_SWM_Deinit();
}

// Initializes the chip to use an external 12MHz crystal as system clock without
// PLL.
static void InitSystemClock() {
  // Start crystal oscillator.
  Chip_SYSCTL_PowerUp(SYSCTL_SLPWAKE_SYSOSC_PD);

  // Crystal oscillator needs up to 500us to start.
  // Wait for 12000/12Mhz=1ms to be sure it is stable before continuing.
  WaitTicks(12000);

  // Select crystal oscillator for main clock.
  Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLIN);

  // Disable internal RC oscillator.
  // TODO: Uncomment when sure that crystal oscillator is working.
  // Chip_SYSCTL_PowerDown(SYSCTL_SLPWAKE_IRCOUT_PD | SYSCTL_SLPWAKE_IRC_PD);
}

// Called by asm setup (startup_LPC82x.s) code before main() is executed.
// Sets up the switch matrix (peripheral to pin connections) and the system
// clock.
extern "C" void SystemInit() {
  InitSwichMatrix();
  InitSystemClock();
}

/// Enables interrupts.
static void SetupNVIC() {
  NVIC_SetPriority(UART0_IRQn, ISR_PRIO_UART);
  NVIC_EnableIRQ(UART0_IRQn);

  NVIC_SetPriority(MRT_IRQn, ISR_PRIO_TIMER);
  NVIC_EnableIRQ(MRT_IRQn);

  NVIC_SetPriority(SCT_IRQn, ISR_PRIO_SCT);
  NVIC_EnableIRQ(SCT_IRQn);
}

// The switch matrix and system clock (12Mhz by external crystal) were already
// configured by SystemInit() before main was called.
extern "C" int main() {
  HwSetupUart(ConfigStorage::Instance().Get(ConfigStorage::kBaudrate));
  HwSetupTimers();
  SetupNVIC();
  MeasureInit();

  uint32_t sensor_id = ConfigStorage::Instance().Get(ConfigStorage::kSlaveId);
  Expect(sensor_id >= 1 && sensor_id <= 247);
  modbus.StartOperation(static_cast<uint8_t>(sensor_id));

  // Main loop.
  for (;;) {
    modbus.Update();

    uint32_t ev = modbus_data.GetEvents();
    if (ev & ModbusData::kWriteConfiguration) {
      ConfigStorage::Instance().WriteConfigToFlash();
    }

    if (ev & ModbusData::kResetDevice) {
      NVIC_SystemReset();
      Expect(false);
    }
  }

  // Never reached.
  return 0;
}
