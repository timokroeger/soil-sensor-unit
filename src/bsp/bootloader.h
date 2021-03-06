// Copyright (c) 2018 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef BSP_BOOTLOADER_H_
#define BSP_BOOTLOADER_H_

#include "bootloader_interface.h"

class Bootloader final : public BootloaderInterface {
 public:
  bool PrepareUpdate() override;
  bool WriteImageData(size_t offset, uint8_t* data, size_t length) override;
  bool SetUpdatePending() override;
  bool SetUpdateConfirmed() override;
};

#endif  // BSP_BOOTLOADER_H_
