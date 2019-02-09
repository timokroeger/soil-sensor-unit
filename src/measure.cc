// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "measure.h"

#include "chip.h"

uint16_t MeasureRaw() {
  Chip_ADC_StartSequencer(LPC_ADC, ADC_SEQA_IDX);

  uint32_t raw_high, raw_low;

  do {
    raw_high = Chip_ADC_GetDataReg(LPC_ADC, 3);
  } while ((raw_high & ADC_SEQ_GDAT_DATAVALID) == 0);

  do {
    raw_low = Chip_ADC_GetDataReg(LPC_ADC, 9);
  } while ((raw_low & ADC_SEQ_GDAT_DATAVALID) == 0);

  return ADC_DR_RESULT(raw_high) - ADC_DR_RESULT(raw_low);
}
