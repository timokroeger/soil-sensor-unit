// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "config_storage.h"

#include <string.h>

#include "chip.h"
#include "config/config_defaults.h"

uint32_t ConfigStorage::config_flash[kNumConfigEntries]
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

void ConfigStorage::Set(ConfigIndex idx, uint32_t value) {
  if (idx >= kMaxConfigIndex) {
    // TODO: Logging
    return;
  }

  config_ram[idx] = value;
}

uint32_t ConfigStorage::Get(ConfigIndex idx) {
  if (idx >= kMaxConfigIndex) {
    // TODO: Logging
    return 0;
  }

  return config_ram[idx];
}

void ConfigStorage::WriteConfigToFlash() {
  Chip_IAP_PreSectorForReadWrite(0, 0);
  Chip_IAP_CopyRamToFlash(reinterpret_cast<uint32_t>(config_flash),
                          config_ram, sizeof(config_ram));
}