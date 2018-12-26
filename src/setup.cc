// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "setup.h"

#include "chip.h"

#include "config.h"

#define ISR_PRIO_UART 1
#define ISR_PRIO_TIMER 1

// Must have highest priority so that the ADC trigger can be reset in time.
#define ISR_PRIO_SCT 0

#define PWM_FREQ 200000u

void SetupGpio() {
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);

  // Set open drain pins as output and pull them low. Also turns on the LED.
  LPC_GPIO_PORT->DIRSET[0] = (1 << 11) | (1 << 10);
  LPC_GPIO_PORT->CLR[0] = (1 << 11) | (1 << 10);

  // Analog mode for ADC input pin.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO23, PIN_MODE_INACTIVE);

  Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_IOCON);
}

void SetupSwichMatrix() {
  // Enable switch matrix.
  Chip_SWM_Init();

  // UART0
  Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 17);
  Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, 12);
  Chip_SWM_MovablePinAssign(SWM_U0_RTS_O, 13);

  // PWM output
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT0_O, 0);   // FREQ_LO
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT1_O, 14);  // FREQ_HI

  // ADC input
  Chip_SWM_EnableFixedPin(SWM_FIXED_ADC3);

  // Switch matrix clock is not needed anymore after configuration.
  Chip_SWM_Deinit();
}

void SetupAdc() {
  Chip_ADC_Init(LPC_ADC, 0);

  // Wait for ADC to be calibrated.
  Chip_ADC_StartCalibration(LPC_ADC);
  while (!Chip_ADC_IsCalibrationDone(LPC_ADC)) continue;

  // Set ADC clock: A value of 0 divides the system clock by 1.
  Chip_ADC_SetDivider(LPC_ADC, 0);

  // Only scan channel 3 when timer triggers the ADC.
  // A conversation takes 25 cycles. Ideally the sampling phase ends right
  // before the PWM output state changes.
  Chip_ADC_SetupSequencer(
      LPC_ADC, ADC_SEQA_IDX,
      ADC_SEQ_CTRL_CHANSEL(3) | (3 << 12) |  // SCT Output 3 as trigger source.
          ADC_SEQ_CTRL_HWTRIG_POLPOS | ADC_SEQ_CTRL_HWTRIG_SYNCBYPASS);
}

void SetupPwm() {
  Chip_SCT_Init(LPC_SCT);

  // Set output frequency with the reload match 0 value. This value is loaded
  // to the match register with each limit event.
  // The timer must expire two times during one PWM cycle (PWM_FREQ * 2) to
  // create complementary outputs with two timer states.
  const uint16_t timer_counts = (CPU_FREQ / (PWM_FREQ * 2));
  static_assert(timer_counts >= 25, "PWM too fast for ADC.");
  LPC_SCT->MATCHREL[0].L = timer_counts - 1;

  // Set ADC trigger 8 cycles (approximate sampling time) before PWM output
  // switches
  // Found by experimenting with different values.
  LPC_SCT->MATCHREL[1].L = timer_counts - 8;

  // Link events to states.
  LPC_SCT->EV[0].STATE = (1 << 0);  // Event 0 only happens in state 0
  LPC_SCT->EV[1].STATE = (1 << 1);  // Event 1 only happens in state 1

  // Add alternating states which are switched by each match event
  LPC_SCT->EV[0].CTRL =
      (1 << 12) |  // COMBMODE[13:12] = Change state on match
      (1 << 14) |  // STATELD[14] = STATEV is loaded into state
      (1 << 15);   // STATEV[19:15] = New state is 1
  LPC_SCT->EV[1].CTRL =
      (1 << 12) |  // COMBMODE[13:12] = Change state on match
      (1 << 14) |  // STATELD[14] = STATEV is loaded into state
      (0 << 15);   // STATEV[19:15] = New state is 0

  // Generate ADC trigger event for each match
  LPC_SCT->EV[3].CTRL = (1 << 0) |  // Use match register 1 for comparison
                        (1 << 12);  // COMBMODE[13:12] = Use only match

  // FREQ_LO: LOW during first half of period, HIGH for the second half.
  LPC_SCT->OUT[0].SET = (1 << 0);  // Event 0 sets
  LPC_SCT->OUT[0].CLR = (1 << 1);  // Event 1 clears

  // FREQ_HI: HIGH during first half of period, LOW for the second half.
  LPC_SCT->OUT[1].SET = (1 << 1);  // Event 1 sets
  LPC_SCT->OUT[1].CLR = (1 << 0);  // Event 0 clears

  // ADC_TRIGGER
  LPC_SCT->OUT[3].SET = (1 << 3);             // Event 3 sets
  LPC_SCT->OUT[3].CLR = (1 << 0) | (1 << 1);  // Event 0 and 1 clears

  // Enable interrupt for event 3 (ADC trigger) to reset disable the trigger
  // again.
  LPC_SCT->EVEN = (1 << 3);

  // Restart counter on event 0 and 1 (match occurred)
  LPC_SCT->LIMIT_L = (1 << 0) | (1 << 1);
}

void SetupTimers() {
  Chip_MRT_Init();

  // Channel 0: MODBUS inter-frame timeout
  Chip_MRT_SetMode(LPC_MRT_CH0, MRT_MODE_ONESHOT);
  Chip_MRT_SetEnabled(LPC_MRT_CH0);
}

void SetupUart(uint32_t baudrate) {
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

void SetupNVIC() {
  NVIC_SetPriority(UART0_IRQn, ISR_PRIO_UART);
  NVIC_EnableIRQ(UART0_IRQn);

  NVIC_SetPriority(MRT_IRQn, ISR_PRIO_TIMER);
  NVIC_EnableIRQ(MRT_IRQn);

  NVIC_SetPriority(SCT_IRQn, ISR_PRIO_SCT);
  NVIC_EnableIRQ(SCT_IRQn);
}
