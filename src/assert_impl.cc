// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "stm32g0xx.h"

extern "C" void __assert_func(const char *, int, const char *, const char *) {
  __disable_irq();
  for (;;);
}
