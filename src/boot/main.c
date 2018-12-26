// Copyright (c) 2018 Timo Kröger <timokroeger93+code@gmail.com>

#include <stdint.h>

#include "bootutil/image.h"
#include "cmsis.h"

extern uint8_t _flash_slot0;

struct arm_vector_table {
  uint32_t msp;
  uint32_t reset;
};

int main() {
  struct image_header *ih = (struct image_header *)&_flash_slot0;
  struct arm_vector_table *vt =
      (struct arm_vector_table *)(&_flash_slot0 + ih->ih_hdr_size);

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
