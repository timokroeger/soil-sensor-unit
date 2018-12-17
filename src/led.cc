// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "led.h"

#include <cassert>

#include "chip.h"
#include "config.h"

static volatile int blink_count = 0;
static volatile bool led_state = true;

void LedBlink(int times) {
  assert(times > 0);

  // Do nothing when blinking already.
  if (blink_count == 0) {
    blink_count = times;
    led_state = true;
    LedTimeout();
  }
}

void LedTimeout() {
  if (led_state) {
    // Turn LED off.
    LPC_GPIO_PORT->B[0][10] = 1;
    led_state = false;

    // Start timer to turn it on again.
    Chip_MRT_SetInterval(LPC_MRT_CH3,
                         ((CPU_FREQ / 1000) * LED_OFF_MS) | MRT_INTVAL_LOAD);
  } else {
    // Turn LED on.
    LPC_GPIO_PORT->B[0][10] = 0;
    led_state = true;

    --blink_count;
    if (blink_count > 0) {
      Chip_MRT_SetInterval(LPC_MRT_CH3,
                           ((CPU_FREQ / 1000) * LED_ON_MS) | MRT_INTVAL_LOAD);
    }
  }
}
