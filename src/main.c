#include <stdbool.h>
#include <stdint.h>

#include "chip.h"

#include "expect.h"

#define OSC_FREQ 12000000u
#define UART_BAUDRATE 115200u
#define PWM_FREQ 200000u

const uint32_t OscRateIn = OSC_FREQ;
const uint32_t ExtRateIn = 0; // External clock input not used.

// Delays execution by a few ticks.
static inline void WaitTicks(uint32_t ticks) {
  uint32_t num_iters = ticks / 3; // ASM code takes 3 ticks for each iteration.
  __asm__ volatile("0: SUB %[i],#1; BNE 0b;" : [i] "+r" (num_iters));
}

void InitSwichMatrix(void)
{
  // Enable switch matrix.
  Chip_SWM_Init();
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);

  // Select XTAL functionality for PIO0_8 and PIO0_9.
  Chip_SWM_EnableFixedPin(SWM_FIXED_XTALIN);
  Chip_SWM_EnableFixedPin(SWM_FIXED_XTALOUT);

  // Disable Pull-Ups on crystal pins.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO8, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO9, PIN_MODE_INACTIVE);

  /*
  // UART0
  Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 10);
  Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, 15);
  Chip_SWM_MovablePinAssign(SWM_U0_RTS_O, 1);
  */

  // PWM output
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT0_O, 0);  // FREQ_LO
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT1_O, 14); // FREQ_HI

  // ADC input
  Chip_SWM_EnableFixedPin(SWM_FIXED_ADC3);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO23, PIN_MODE_INACTIVE);

  // Switch matrix clock is not needed anymore after configuration.
  Chip_SWM_Deinit();
}

// Initializes the chip to use an external 12MHz crystal as system clock without
// PLL.
void InitSystemClock(void)
{
  // Start crystal oscillator.
  Chip_SYSCTL_PowerUp(SYSCTL_SLPWAKE_SYSOSC_PD);

  // Crystal oscillator needs up to 500us to start.
  // Wait for 12000/12Mhz=1ms to be sure it is stable before continuing.
  WaitTicks(12000);

  // Select crystal oscillator for main clock.
  Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLIN);

  // Disable internal RC oscillator.
  // TODO: Uncomment when sure that crystal oscillator is working.
  //Chip_SYSCTL_PowerDown(SYSCTL_SLPWAKE_IRCOUT_PD | SYSCTL_SLPWAKE_IRC_PD);
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
static void SetupUart(void)
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

// Setup a PWM output with 50% duty cycle.
static void SetupPWM(void)
{
  Chip_SCT_Init(LPC_SCT);

  // Set output frequency with the reload match 0 value. This value is loaded
  // to the match register with each limit event.
  // The timer must expire two times during one PWM cycle (PWM_FREQ * 2) to
  // create complementary outputs with two timer states.
  uint16_t timer_counts = (OSC_FREQ / (PWM_FREQ * 2)) - 1;
  LPC_SCT->MATCHREL[0].L = timer_counts;

  // Set ADC trigger 12 cycles (approximate sampling time) before PWM output switches
  Expect(timer_counts > 12);
  LPC_SCT->MATCHREL[1].L = timer_counts - 12;

  // Link events to states.
  LPC_SCT->EV[0].STATE = 1; // Event 0 only happens in state 0
  LPC_SCT->EV[1].STATE = 2; // Event 1 only happens in state 1
  LPC_SCT->EV[3].STATE = 3; // Event 3 happens in state 0 and 1

  // Add alternating states which are switched by each match event
  LPC_SCT->EV[0].CTRL = (1 << 12) | // COMBMODE[13:12] = Change state on match
                        (1 << 14) | // STATELD[14] = STATEV is loaded into state
                        (1 << 15);  // STATEV[19:15] = New state is 1
  LPC_SCT->EV[1].CTRL = (1 << 12) | // COMBMODE[13:12] = Change state on match
                        (1 << 14) | // STATELD[14] = STATEV is loaded into state
                        (0 << 15);  // STATEV[19:15] = New state is 0

  // Generate ADC trigger event for each match
  LPC_SCT->EV[3].CTRL = (1 << 0) | // Use match register 1 for comparison
                        (1 << 12); // COMBMODE[13:12] = Use only match

  // FREQ_LO: LOW during first half of period, HIGH for the second half.
  LPC_SCT->OUT[0].SET = (1 << 1); // Event 1 sets
  LPC_SCT->OUT[0].CLR = (1 << 0); // Event 0 clears

  // FREQ_HI: HIGH during first half of period, LOW for the second half.
  LPC_SCT->OUT[1].SET = (1 << 0); // Event 0 sets
  LPC_SCT->OUT[1].CLR = (1 << 1); // Event 1 clears

  // ADC_TRIGGER
  LPC_SCT->OUT[3].SET = (1 << 3);            // Event 3 sets
  LPC_SCT->OUT[3].CLR = (1 << 0) | (1 << 1); // Event 0 and 1 clears

  // Restart counter on event 0 and 1 (match occurred)
  LPC_SCT->LIMIT_L = 3;
}

static void StartPWM(void)
{
  // Start timer.
  LPC_SCT->CTRL_L &= ~SCT_CTRL_HALT_L;
}

static void SetupADC(void)
{
  Chip_ADC_Init(LPC_ADC, 0);

  // Wait for ADC to be calibrated.
  Chip_ADC_StartCalibration(LPC_ADC);
  while (!Chip_ADC_IsCalibrationDone(LPC_ADC));

  // Set ADC clock: A value of 0 divides the system clock by 1.
  Chip_ADC_SetDivider(LPC_ADC, 0);

  // Only scan channel 3 when timer triggers the ADC.
  // A conversation takes 25 cycles. Ideally the sampling phase ends right
  // before the PWM output state changes.
  Chip_ADC_SetupSequencer(LPC_ADC, ADC_SEQA_IDX,
                          ADC_SEQ_CTRL_CHANSEL(3) |
                          (3 << 12) | // SCT Output 3 as trigger source.
                          ADC_SEQ_CTRL_HWTRIG_POLPOS |
                          ADC_SEQ_CTRL_HWTRIG_SYNCBYPASS);
  Chip_ADC_EnableSequencer(LPC_ADC, ADC_SEQA_IDX);

  Chip_ADC_EnableInt(LPC_ADC, ADC_INTEN_SEQA_ENABLE);
}

static void SetupNVIC(void)
{
  NVIC_SetPriority(ADC_SEQA_IRQn, 0);
  NVIC_EnableIRQ(ADC_SEQA_IRQn);
}

void ADC_SEQA_IRQHandler(void)
{
  Expect(Chip_ADC_GetFlags(LPC_ADC) == ADC_FLAGS_SEQA_INT_MASK);
  Chip_ADC_ClearFlags(LPC_ADC, ADC_FLAGS_SEQA_INT_MASK);

  uint32_t adc_value_raw = Chip_ADC_GetSequencerDataReg(LPC_ADC, ADC_SEQA_IDX);
  Expect(adc_value_raw & ADC_SEQ_GDAT_DATAVALID);

  uint32_t adc_value = ADC_DR_RESULT(adc_value_raw);
  // TODO: Check value
}

// The switch matrix and system clock (12Mhz by external crystal) were already
// configured by SystemInit() before main was called.
int main(void)
{
  //SetupUart();
  SetupPWM();
  SetupADC();
  SetupNVIC();

  StartPWM();

  // Main loop.
  for (;;) {
    // TODO
  }

  // Never reached.
  return 0;
}
