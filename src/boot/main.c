// Copyright (c) 2018 Timo Kröger <timokroeger93+code@gmail.com>

#include "cmsis.h"

struct arm_vector_table {
  uint32_t msp;
  uint32_t reset;
};

int main() {
  // Hardcode slot0 address for now.
  struct arm_vector_table *vt = (struct arm_vector_table *)0x00002100;

  // Update vector table.
  SCB->VTOR = (uint32_t)vt;

  // Load new stack pointer.
  __set_MSP(vt->msp);

  // Jump to actual reset handler.
  ((void (*)(void))vt->reset)();

  for (;;)
    ;

  // Not reached.
  return 0;
}
