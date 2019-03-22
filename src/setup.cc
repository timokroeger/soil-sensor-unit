// Copyright (c) 2019 Timo Kröger <timokroeger93+code@gmail.com>

#include "setup.h"

#include <cassert>

#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_tim.h"

namespace {

constexpr uint32_t kTimerClkFrequency = 128'000'000;
constexpr uint32_t kTimerPwmFrequency = 64'000'000;

void SetupBus() {
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR | LL_APB1_GRP1_PERIPH_USART2);
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
}

// Configures PLLQCLK to output 128MHz for TIM1.
void SetupPll() {
  // PLL must not be running when modifying registers.
  assert(!LL_RCC_PLL_IsReady());

  // 128MHz PLLQCLK is only possible with voltage scaling range 1.
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

void SetupTim1() {
  assert(LL_RCC_GetTIMClockFreq(LL_RCC_TIM1_CLKSOURCE) == kTimerClkFrequency);

  LL_TIM_InitTypeDef tim1_init;
  LL_TIM_StructInit(&tim1_init);
  tim1_init.Autoreload = (kTimerClkFrequency / kTimerPwmFrequency) - 1;
  LL_TIM_Init(TIM1, &tim1_init);

  // Setup a complementary 50% duty cycle PWM.
  LL_TIM_OC_InitTypeDef tim1_oc_init;
  LL_TIM_OC_StructInit(&tim1_oc_init);
  tim1_oc_init.CompareValue = (kTimerClkFrequency / kTimerPwmFrequency) / 2;

  tim1_oc_init.OCMode = LL_TIM_OCMODE_PWM1;
  tim1_oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
  tim1_oc_init.OCNState = LL_TIM_OCSTATE_DISABLE;
  LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH1, &tim1_oc_init);

  tim1_oc_init.OCMode = LL_TIM_OCMODE_PWM2;
  tim1_oc_init.OCState = LL_TIM_OCSTATE_DISABLE;
  tim1_oc_init.OCNState = LL_TIM_OCSTATE_ENABLE;
  LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH3, &tim1_oc_init);

  LL_TIM_EnableCounter(TIM1);
}

}  // namespace

void Setup() {
  SetupBus();
  SetupPll();
  SetupTim1();
}
