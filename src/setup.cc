// Copyright (c) 2019 Timo Kröger <timokroeger93+code@gmail.com>

#include "setup.h"

#include <cassert>

#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_gpio.h"
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

void SetupGpio() {
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB | LL_IOP_GRP1_PERIPH_GPIOA);

  // TIM
  LL_GPIO_InitTypeDef gpio_tim_init;
  gpio_tim_init.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio_tim_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_tim_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_tim_init.Pull = LL_GPIO_PULL_NO;
  gpio_tim_init.Alternate = LL_GPIO_AF_2;

  // TIM1_CH1 - PB1
  gpio_tim_init.Pin = LL_GPIO_PIN_1;
  LL_GPIO_Init(GPIOB, &gpio_tim_init);

  // TIM1_CH3N - PA8
  gpio_tim_init.Pin = LL_GPIO_PIN_8;
  LL_GPIO_Init(GPIOA, &gpio_tim_init);

  // USART
  LL_GPIO_InitTypeDef gpio_usart_init;
  gpio_usart_init.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio_usart_init.Speed = LL_GPIO_SPEED_FREQ_LOW;
  gpio_usart_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_usart_init.Pull = LL_GPIO_PULL_NO;
  gpio_usart_init.Alternate = LL_GPIO_AF_1;

  // USART2_DE - PA1
  gpio_usart_init.Pin = LL_GPIO_PIN_1;
  LL_GPIO_Init(GPIOA, &gpio_usart_init);

  // USART2_TX - PA2
  gpio_usart_init.Pin = LL_GPIO_PIN_2;
  LL_GPIO_Init(GPIOA, &gpio_usart_init);

  // USART2_RX - PA3
  gpio_usart_init.Pin = LL_GPIO_PIN_3;
  gpio_usart_init.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOA, &gpio_usart_init);
}

}  // namespace

void Setup() {
  SetupBus();
  SetupPll();
  SetupTim1();
  SetupGpio();
}
