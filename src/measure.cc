// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "measure.h"

#include "chip.h"

#include "common.h"
#include "expect.h"

#define PWM_FREQ 200000u

static void SetupAdc(void) {
  Chip_ADC_Init(LPC_ADC, 0);

  // Wait for ADC to be calibrated.
  Chip_ADC_StartCalibration(LPC_ADC);
  while (!Chip_ADC_IsCalibrationDone(LPC_ADC))
    ;

  // Set ADC clock: A value of 0 divides the system clock by 1.
  Chip_ADC_SetDivider(LPC_ADC, 0);

  // Only scan channel 3 when timer triggers the ADC.
  // A conversation takes 25 cycles. Ideally the sampling phase ends right
  // before the PWM output state changes.
  Chip_ADC_SetupSequencer(
      LPC_ADC, ADC_SEQA_IDX,
      ADC_SEQ_CTRL_CHANSEL(3) | (3 << 12) |  // SCT Output 3 as trigger source.
          ADC_SEQ_CTRL_HWTRIG_POLPOS | ADC_SEQ_CTRL_HWTRIG_SYNCBYPASS);
  Chip_ADC_EnableSequencer(LPC_ADC, ADC_SEQA_IDX);
}

// Setup a PWM output with 50% duty cycle.
static void SetupPwm(void) {
  Chip_SCT_Init(LPC_SCT);

  // Set output frequency with the reload match 0 value. This value is loaded
  // to the match register with each limit event.
  // The timer must expire two times during one PWM cycle (PWM_FREQ * 2) to
  // create complementary outputs with two timer states.
  const uint16_t timer_counts = (OSC_FREQ / (PWM_FREQ * 2)) - 1;
  static_assert(timer_counts > 25, "PWM too fast for ADC.");
  LPC_SCT->MATCHREL[0].L = timer_counts;

  // Set ADC trigger 8 cycles (approximate sampling time) before PWM output
  // switches
  // Found by experimenting with different values.
  LPC_SCT->MATCHREL[1].L = (uint16_t)(timer_counts - 8);

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

    // Start timer.
  LPC_SCT->CTRL_L &= (uint16_t)~SCT_CTRL_HALT_L;
}

void MeasureInit(void) {
  SetupAdc();
  SetupPwm();
}

uint16_t MeasureRaw(bool high) {
  // ADC Trigger is event 3 of the timer. Select state the event occurs.
  LPC_SCT->EV[3].STATE = high ? (1 << 0)   // Sample on HI level: State 0
                              : (1 << 1);  // Sample on LO level: State 1

  // Wait for ADC conversion to finish.
  uint32_t adc_value_raw;
  do {
    adc_value_raw = Chip_ADC_GetSequencerDataReg(LPC_ADC, ADC_SEQA_IDX);
  } while ((adc_value_raw & ADC_SEQ_GDAT_DATAVALID) == 0);

  return ADC_DR_RESULT(adc_value_raw);
}

extern "C" void SCT_IRQHandler(void) {
  // Disable ADC trigger immediately so it is not re-triggered accidentally.
  LPC_SCT->EV[3].STATE = 0;

  // Clear interrupt flag.
  LPC_SCT->EVFLAG = (1 << 3);
}
