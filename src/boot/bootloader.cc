// Copyright (c) 2018 Timo Kröger <timokroeger93+code@gmail.com>

#include "bootloader.h"

#include "bootutil/bootutil.h"
#include "flash_map_backend/flash_map_backend.h"
#include "sysflash/sysflash.h"

Bootloader::Bootloader() { flash_areas_init(); }

bool Bootloader::PrepareUpdate() {
  const struct flash_area *fa;
  int rc = flash_area_open(FLASH_AREA_IMAGE_1, &fa);
  if (rc != 0) {
    return false;
  }

  rc = flash_area_erase(fa, 0, fa->fa_size);
  if (rc != 0) {
    flash_area_close(fa);
    return false;
  }

  flash_area_close(fa);
  return true;
}

bool Bootloader::WriteImageData(size_t offset, uint8_t *data, size_t length) {
  const struct flash_area *fa;
  int rc = flash_area_open(FLASH_AREA_IMAGE_1, &fa);
  if (rc != 0) {
    return false;
  }

  rc = flash_area_write(fa, offset, data, length);
  if (rc != 0) {
    flash_area_close(fa);
    return false;
  }

  flash_area_close(fa);
  return true;
}

bool Bootloader::SetUpdatePending() { return boot_set_pending(0) == 0; }

bool Bootloader::SetUpdateConfirmed() { return boot_set_confirmed() == 0; }
