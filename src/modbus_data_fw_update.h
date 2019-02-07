// Copyright (c) 2019 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef FW_UPDATE_
#define FW_UPDATE_

#include <cstdint>

#include "etl/vector.h"

#include "bootloader_interface.h"
#include "modbus/data_interface.h"

class ModbusDataFwUpdate final : public modbus::DataInterface {
 public:
  static constexpr uint16_t kCommandRegister = 0x7FFF;

  enum Command : uint16_t {
    kPrepare,
    kSetPending,
    kConfirm,
  };

  explicit ModbusDataFwUpdate(BootloaderInterface& bootloader)
      : bootloader_(bootloader) {}

  bool ReadRegister(uint16_t address, uint16_t* data_out) override;
  bool WriteRegister(uint16_t address, uint16_t data) override;

 private:
  static constexpr size_t kBufferSize = 1024;

  void WriteBuffer();

  BootloaderInterface& bootloader_;

  etl::vector<uint8_t, kBufferSize> write_buffer_;
  size_t write_offset_ = 0;
};

#endif  // FW_UPDATE_
