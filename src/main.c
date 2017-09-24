#include <stdbool.h>
#include <stdint.h>

#include "chip.h"

#include "expect.h"
#include "log.h"
#include "modbus.h"

#define OSC_FREQ 12000000u
#define UART_BAUDRATE 19200u
#define PWM_FREQ 200000u

#define ISR_PRIO_UART 1
#define ISR_PRIO_TIMER 1

// Must have highest priority so that the ADC trigger can be reset in time.
#define ISR_PRIO_SCT  0

const uint32_t OscRateIn = OSC_FREQ;
const uint32_t ExtRateIn = 0; // External clock input not used.

// Increased when no stop bits were received.
static uint32_t uart_frame_error_counter = 0;

// No parity bit is used in the GaMoSy Communication Protocol so this should
// always stay at 0.
static uint32_t uart_parity_error_counter = 0;

// Increased when only 2 of 3 samples of a UART bit reading were stable.
static uint32_t uart_noise_error_counter = 0;

// Delays execution by a few ticks.
static inline void WaitTicks(uint32_t ticks) {
  uint32_t num_iters = ticks / 3; // ASM code takes 3 ticks for each iteration.
  __asm__ volatile("0: SUB %[i],#1; BNE 0b;" : [i] "+r" (num_iters));
}

void UART0_IRQHandler(void)
{
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

  uint8_t rxdata = Chip_UART_ReadByte(LPC_USART0);
  ModbusByteReceived(rxdata);

  Chip_UART_ClearStatus(LPC_USART0,
                        UART_STAT_RXRDY | UART_STAT_FRM_ERRINT |
                            UART_STAT_PAR_ERRINT | UART_STAT_RXNOISEINT);
}

void MRT_IRQHandler(void)
{
  if (Chip_MRT_IntPending(LPC_MRT_CH0)) {
    Chip_MRT_IntClear(LPC_MRT_CH0);
    ModbusTimeout(kModbusTimeoutInterCharacterDelay);
  }

  if (Chip_MRT_IntPending(LPC_MRT_CH1)) {
    Chip_MRT_IntClear(LPC_MRT_CH1);
    ModbusTimeout(kModbusTimeoutInterFrameDelay);
  }
}

void SCT_IRQHandler(void)
{
  // Disable ADC trigger immediately so it is not re-triggered accidentally.
  LPC_SCT->EV[3].STATE = 0;

  // Clear interrupt flag.
  LPC_SCT->EVFLAG = (1 << 3);
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

  // UART0
  Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 4);
  Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, 15);
  Chip_SWM_MovablePinAssign(SWM_U0_RTS_O, 1);

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
static void SetupUART(void)
{
  // Enable global UART clock.
  Chip_Clock_SetUARTClockDiv(1);

  Chip_UART_Init(LPC_USART0);
  Chip_UART_SetBaud(LPC_USART0, UART_BAUDRATE);
  Chip_UART_ConfigData(LPC_USART0, UART_CFG_ENABLE | UART_CFG_DATALEN_8 |
      UART_CFG_PARITY_EVEN | UART_CFG_STOPLEN_1 | UART_CFG_OESEL | UART_CFG_OEPOL);

  // Enable receive and overrun interrupt. No interrupts for frame, parity or
  // noise errors are enabled because those are checked when reading a received
  // byte.
  Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY | UART_INTEN_OVERRUN);
}

static void SetupTimers(void)
{
  Chip_MRT_Init();

  // Channel 0: MODBUS inter-character timeout
  Chip_MRT_SetMode(LPC_MRT_CH0, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH0);

  // Channel 1: MODBUS inter-frame timeout
  Chip_MRT_SetMode(LPC_MRT_CH1, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH1);
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

  // Set ADC trigger 8 cycles (approximate sampling time) before PWM output switches
  // Found by experimenting with different values.
  Expect(timer_counts > 8);
  LPC_SCT->MATCHREL[1].L = timer_counts - 8;

  // Link events to states.
  LPC_SCT->EV[0].STATE = (1 << 0); // Event 0 only happens in state 0
  LPC_SCT->EV[1].STATE = (1 << 1); // Event 1 only happens in state 1

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
  LPC_SCT->OUT[0].SET = (1 << 0); // Event 0 sets
  LPC_SCT->OUT[0].CLR = (1 << 1); // Event 1 clears

  // FREQ_HI: HIGH during first half of period, LOW for the second half.
  LPC_SCT->OUT[1].SET = (1 << 1); // Event 1 sets
  LPC_SCT->OUT[1].CLR = (1 << 0); // Event 0 clears

  // ADC_TRIGGER
  LPC_SCT->OUT[3].SET = (1 << 3);            // Event 3 sets
  LPC_SCT->OUT[3].CLR = (1 << 0) | (1 << 1); // Event 0 and 1 clears

  // Enable interrupt for event 3 (ADC trigger) to reset disable the trigger again.
  LPC_SCT->EVEN = (1 << 3);

  // Restart counter on event 0 and 1 (match occurred)
  LPC_SCT->LIMIT_L = (1 << 0) | (1 << 1);
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
}

static uint16_t ReadADC(bool high)
{
  // ADC Trigger is event 3 of the timer. Select state the event occurs.
  LPC_SCT->EV[3].STATE = high
                       ? (1 << 0)  // Sample on HI level: State 0
                       : (1 << 1); // Sample on LO level: State 1

  // Wait for ADC conversion to finish.
  uint32_t adc_value_raw;
  do {
    adc_value_raw = Chip_ADC_GetSequencerDataReg(LPC_ADC, ADC_SEQA_IDX);
  } while ((adc_value_raw & ADC_SEQ_GDAT_DATAVALID) == 0);

  return ADC_DR_RESULT(adc_value_raw);
}

/// Enables interrupts.
static void SetupNVIC(void)
{
  Expect(Chip_UART_GetIntsEnabled(LPC_USART0) != 0);
  NVIC_SetPriority(UART0_IRQn, ISR_PRIO_UART);
  NVIC_EnableIRQ(UART0_IRQn);

  NVIC_SetPriority(MRT_IRQn, ISR_PRIO_TIMER);
  NVIC_EnableIRQ(MRT_IRQn);

  NVIC_SetPriority(SCT_IRQn, ISR_PRIO_SCT);
  NVIC_EnableIRQ(SCT_IRQn);
}

// The switch matrix and system clock (12Mhz by external crystal) were already
// configured by SystemInit() before main was called.
int main(void)
{
  SetupUART();
  SetupTimers();
  Chip_CRC_Init();
  SetupPWM();
  SetupADC();
  SetupNVIC();

  StartPWM();

  ModbusSetAddress(1);
  ModbusStartTimer(); // TODO: Remove

  // Main loop.
  for (;;) {
    uint32_t tmp = 0;
    for (int i = 0; i < (1 << 16); i++) {
      uint32_t low = ReadADC(false);
      uint32_t high = ReadADC(true);
      tmp += high - low;
    }
    tmp >>= 16;
    LOG_DEBUG("%d\n", tmp);
  }

  // Never reached.
  return 0;
}
