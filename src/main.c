#include <stdbool.h>
#include <stdint.h>

#include "chip.h"

#include "expect.h"

#define OSC_FREQ 12000000u
#define UART_BAUDRATE 115200u

const uint32_t OscRateIn = OSC_FREQ;
const uint32_t ExtRateIn = 0; // External clock input not used.

// Delays execution by a few ticks.
static inline void WaitTicks(uint32_t ticks) {
  uint32_t num_iters = ticks / 3; // ASM code takes 3 ticks for each iteration.
  asm volatile("0: SUB %[i],#1; BNE 0b;" : [i] "+r" (num_iters));
}

void InitSwichMatrix(void)
{
  // Enable switch matrix.
  Chip_SWM_Init();

  // Select XTAL functionality for PIO0_8 and PIO0_9.
  Chip_SWM_EnableFixedPin(SWM_FIXED_XTALIN);
  Chip_SWM_EnableFixedPin(SWM_FIXED_XTALOUT);

  // UART0
  Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 10);
  Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, 15);
  Chip_SWM_MovablePinAssign(SWM_U0_RTS_O, 1);

  // Switch matrix clock is not needed anymore after configuration.
  Chip_SWM_Deinit();
}

// Initializes the chip to use an external 12MHz crystal as system clock without
// PLL.
void InitSystemClock(void)
{
  // Disable Pull-Ups on crystal pins.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO8, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO9, PIN_MODE_INACTIVE);

  // Start crystal oscillator.
  Chip_SYSCTL_PowerUp(SYSCTL_SLPWAKE_SYSOSC_PD);

  // Crystal oscillator needs up to 500us to start.
  // Wait for 7500/12Mhz=625us to be sure it is stable before continuing.
  WaitTicks(7500);

  // Select crystal oscillator for main clock.
  Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLIN);
}

// Called by asm setup (startup_LPC82x.s) code before main() is executed.
// Sets up the switch matrix (peripheral to pin connections) and the system
// clock.
void SystemInit(void)
{
  InitSwichMatrix();
  InitSystemClock();
}

// Setup UART0 with RTS pin as drive enable for the RS485 receiver.
void SetupUart(void)
{
  Chip_UART_Init(LPC_USART0);

  // TODO: Enable interrupts.

  // Use fractional divider to create a UART base frequency of
  // 12MHz / (1 + 161/256) = 7366906.475Hz which is approximately gives us a
  // baud rate of 7366906.475Hz/16/4 = 115108Hz (<0.1% error of 115200Hz)
  Chip_Clock_SetUARTClockDiv(1);
  Chip_SYSCTL_SetUSARTFRGDivider(0xFF);
  Chip_SYSCTL_SetUSARTFRGMultiplier(161);

  Chip_UART_SetBaud(LPC_USART0, UART_BAUDRATE);
  Chip_UART_ConfigData(LPC_USART0, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE |
      UART_CFG_STOPLEN_1 | UART_CFG_OESEL);
}

// The switch matrix and system clock (12Mhz by external crystal) were already
// configured by SystemInit() before main was called.
int main(void)
{
  SetupUart();

  // Main loop.
  for (;;) {
    // TODO
  }

  // Never reached.
  return 0;
}
