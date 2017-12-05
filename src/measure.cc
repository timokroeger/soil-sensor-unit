// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "measure.h"

#include "chip.h"

void MeasureStart() {
  Chip_ADC_EnableSequencer(LPC_ADC, ADC_SEQA_IDX);
  LPC_SCT->CTRL_L &= (uint16_t)~SCT_CTRL_HALT_L;
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

void MeasureResetTrigger() {
  // Disable ADC trigger immediately so it is not re-triggered accidentally.
  LPC_SCT->EV[3].STATE = 0;

  // Clear interrupt flag.
  LPC_SCT->EVFLAG = (1 << 3);
}
