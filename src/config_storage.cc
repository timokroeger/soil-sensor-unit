// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "config_storage.h"

#include <cstring>

#include "chip.h"
#include "config/config_defaults.h"

uint16_t ConfigStorage::config_flash[kNumConfigEntries]
    __attribute__((section(".config_flash"))) = {
        [kSlaveId] = CONFIG_DEFAULT_SENSOR_ID,
        [kBaudrate] = CONFIG_DEFAULT_BAUDRATE,
};

ConfigStorage::ConfigStorage() {
  memcpy(config_ram, config_flash, sizeof(config_ram));
}

ConfigStorage &ConfigStorage::Instance() {
  static ConfigStorage cs;
  return cs;
}

void ConfigStorage::Set(ConfigIndex idx, uint16_t value) {
  if (idx >= kMaxConfigIndex) {
    // TODO: Logging
    return;
  }

  config_ram[idx] = value;
}

uint16_t ConfigStorage::Get(ConfigIndex idx) {
  if (idx >= kMaxConfigIndex) {
    // TODO: Logging
    return 0;
  }

  return config_ram[idx];
}

void ConfigStorage::WriteConfigToFlash() {
  // 1 sector = 1kb
  uint32_t sector_nr = reinterpret_cast<uint32_t>(&config_flash[0]) / 1024;
  // 1 page = 64b
  uint32_t page_nr = reinterpret_cast<uint32_t>(&config_flash[0]) / 64; 

  __disable_irq();
  // Erase old page
  Chip_IAP_PreSectorForReadWrite(sector_nr, sector_nr);
  Chip_IAP_ErasePage(page_nr, page_nr);

  // Write new page
  Chip_IAP_PreSectorForReadWrite(sector_nr, sector_nr);
  Chip_IAP_CopyRamToFlash(reinterpret_cast<uint32_t>(config_flash),
                          reinterpret_cast<uint32_t *>(config_ram),
                          sizeof(config_ram));
  __enable_irq();
}
