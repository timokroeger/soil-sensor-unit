// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef CONFIG_STORAGE_
#define CONFIG_STORAGE_

#include <stdint.h>

class ConfigStorage {
 public:
  enum ConfigIndex {
    kSlaveId = 0,
    kBaudrate,
    kMaxConfigIndex,
  };

  static ConfigStorage& Instance();

  void Set(ConfigIndex idx, uint16_t value);
  uint16_t Get(ConfigIndex idx);

  void WriteConfigToFlash();

 private:
  ConfigStorage();

  static const int kNumConfigEntries = 32;

  static uint16_t config_flash[kNumConfigEntries]
      __attribute__((section(".config_flash")));

  uint16_t config_ram[kNumConfigEntries];
};

#endif  // CONFIG_STORAGE_