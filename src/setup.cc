// Copyright (c) 2019 Timo Kröger <timokroeger93+code@gmail.com>

#include "setup.h"

#include <cassert>

#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_rcc.h"

namespace {

// Configures PLLQCLK to output 128MHz for TIM1.
void SetupPll() {
  // PLL must not be running when modifying registers.
  assert(!LL_RCC_PLL_IsReady());

  // 128MHz PLLQCLK is only possible with voltage scaling range 1.
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  assert(LL_PWR_GetRegulVoltageScaling() == LL_PWR_REGU_VOLTAGE_SCALE1);

  LL_RCC_PLL_ConfigDomain_TIM1(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 16,
                               LL_RCC_PLLQ_DIV_2);
  LL_RCC_PLL_EnableDomain_TIM1();

  // Start PLL with IRC as source
  LL_RCC_PLL_Enable();
  while (!LL_RCC_PLL_IsReady())
    ;

  LL_RCC_SetTIMClockSource(LL_RCC_TIM1_CLKSOURCE_PLL);
}

}  // namespace

void Setup() {
  SetupPll();
}
