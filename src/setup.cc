// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "setup.h"

#include "chip.h"

#define ISR_PRIO_UART 1
#define ISR_PRIO_TIMER 1

#define PWM_FREQ 200000u

// Required by the vendor chip library.
const uint32_t OscRateIn = 0;  // External oscillator not used.
const uint32_t ExtRateIn = 0;  // External clock input not used.

ModbusSerial modbus_serial(LPC_USART0, LPC_MRT_CH0);

void SetupClock() {
  // Use vendor provided routine in ROM memory to setup the system clock.
  // It uses alsmost 1kb less flash compared to the version Chip_IRC_SetFreq()
  // shipped in the lpc_chip_82x libraries.
  bool ok = Chip_IRC_SetFreq(MAIN_FREQ, SYSTEM_FREQ);

  // Update CMSIS clock frequency variable which is used in iap.c
  SystemCoreClock = SYSTEM_FREQ;
}

void SetupGpio() {
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);

  // Set open drain pins as output and pull them low. Also turns on the LED.
  LPC_GPIO_PORT->DIRSET[0] = (1 << 11) | (1 << 10);
  LPC_GPIO_PORT->CLR[0] = (1 << 11) | (1 << 10);

  // Analog mode for ADC input pin.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO17, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO23, PIN_MODE_INACTIVE);

  Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_IOCON);
}

void SetupSwichMatrix() {
  // Enable switch matrix.
  Chip_SWM_Init();

  // UART0
  Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 15);
  Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, 9);
  Chip_SWM_MovablePinAssign(SWM_U0_RTS_O, 1);

  // PWM output
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT0_O, 0);   // FREQ_LO
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT1_O, 14);  // FREQ_HI

  // ADC input
  Chip_SWM_EnableFixedPin(SWM_FIXED_ADC3);
  Chip_SWM_EnableFixedPin(SWM_FIXED_ADC9);

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
  Chip_ADC_SetupSequencer(LPC_ADC, ADC_SEQA_IDX,
                          ADC_SEQ_CTRL_CHANSEL(3) | ADC_SEQ_CTRL_CHANSEL(9) |
                              ADC_SEQ_CTRL_HWTRIG_POLPOS);
  Chip_ADC_EnableSequencer(LPC_ADC, ADC_SEQA_IDX);
}

void SetupPwm() {
  Chip_SCT_Init(LPC_SCT);

  // Set output frequency with the reload match 0 value. This value is loaded
  // to the match register with each limit event.
  // The timer must expire two times during one PWM cycle (PWM_FREQ * 2) to
  // create complementary outputs with two timer states.
  // Use the fastet possible pwm frequency by setting the reload value to 0:
  // 30MHz / 2 = 15MHz
  LPC_SCT->MATCHREL[0].L = 0;

  // Link events to states.
  LPC_SCT->EV[0].STATE = (1 << 0);  // Event 0 only happens in state 0
  LPC_SCT->EV[1].STATE = (1 << 1);  // Event 1 only happens in state 1

  // Add alternating states which are switched by each match event
  LPC_SCT->EV[0].CTRL =
      (0 << 0) |   // Use match register 0 for comparison
      (1 << 12) |  // COMBMODE[13:12] = Change state on match
      (1 << 14) |  // STATELD[14] = STATEV is loaded into state
      (1 << 15);   // STATEV[19:15] = New state is 1
  LPC_SCT->EV[1].CTRL =
      (0 << 0) |   // Use match register 0 for comparison
      (1 << 12) |  // COMBMODE[13:12] = Change state on match
      (1 << 14) |  // STATELD[14] = STATEV is loaded into state
      (0 << 15);   // STATEV[19:15] = New state is 0

  // FREQ_LO: LOW during first half of period, HIGH for the second half.
  LPC_SCT->OUT[0].SET = (1 << 0);  // Event 0 sets
  LPC_SCT->OUT[0].CLR = (1 << 1);  // Event 1 clears

  // FREQ_HI: HIGH during first half of period, LOW for the second half.
  LPC_SCT->OUT[1].SET = (1 << 1);  // Event 1 sets
  LPC_SCT->OUT[1].CLR = (1 << 0);  // Event 0 clears

  // Restart counter on event 0 and 1 (match occurred)
  LPC_SCT->LIMIT_L = (1 << 0) | (1 << 1);

  // Start the timer
  LPC_SCT->CTRL_L &= (uint16_t)~SCT_CTRL_HALT_L;
}

void SetupTimers() { Chip_MRT_Init(); }

void SetupNVIC() {
  NVIC_SetPriority(UART0_IRQn, ISR_PRIO_UART);
  NVIC_EnableIRQ(UART0_IRQn);

  NVIC_SetPriority(MRT_IRQn, ISR_PRIO_TIMER);
  NVIC_EnableIRQ(MRT_IRQn);
}
