// Copyright (c) 2018 Timo Kröger <timokroeger93+code@gmail.com>

#include "bootutil/bootutil.h"
#include "bootutil/image.h"
#include "stm32g0xx.h"

struct arm_vector_table {
  uint32_t msp;
  uint32_t reset;
};

int main() {
  struct boot_rsp rsp;
  int rc = boot_go(&rsp);

  if (rc == 0) {
    struct arm_vector_table *vt =
        (struct arm_vector_table *)(rsp.br_image_off + rsp.br_hdr->ih_hdr_size);

    // Load new stack pointer.
    __set_MSP(vt->msp);

    // Jump to actual reset handler.
    ((void (*)(void))vt->reset)();
  }

  for (;;)
    ;

  // Not reached.
  return 0;
}
