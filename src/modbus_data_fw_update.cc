// Copyright (c) 2019 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_data_fw_update.h"

modbus::ExceptionCode ModbusDataFwUpdate::ReadRegister(uint16_t address,
                                                       uint16_t* data_out) {
  // All fw update registers are write-only.
  return modbus::ExceptionCode::kIllegalDataAddress;
};

modbus::ExceptionCode ModbusDataFwUpdate::WriteRegister(uint16_t address,
                                                        uint16_t data) {
  if (address == kCommandRegister) {
    bool ok = false;

    switch (data) {
      case Command::kPrepare:
        ok = bootloader_.PrepareUpdate();
        write_buffer_.clear();
        write_offset_ = 0;
        break;

      case Command::kSetPending:
        // Write remaining bytes to permanent storage before flagging the
        // update as pending.
        if (!write_buffer_.empty()) {
          write_buffer_.resize(write_buffer_.capacity(), 0xFF);
          WriteBuffer();
        }
        ok = bootloader_.SetUpdatePending();
        break;

      case Command::kConfirm:
        ok = bootloader_.SetUpdateConfirmed();
        break;
    }

    return ok ? modbus::ExceptionCode::kOk
              : modbus::ExceptionCode::kIllegalDataValue;
  }

  // Assume that the client sends the firmware image data in sequence. If not
  // the signature check will fail anyway.
  write_buffer_.push_back(data >> 8);
  write_buffer_.push_back(data & 0xFF);
  if (write_buffer_.full()) {
    WriteBuffer();
  }

  return modbus::ExceptionCode::kOk;
}

void ModbusDataFwUpdate::WriteBuffer() {
  bootloader_.WriteImageData(write_offset_, write_buffer_.data(),
                             write_buffer_.size());
  write_offset_ += write_buffer_.size();
  write_buffer_.clear();
}
