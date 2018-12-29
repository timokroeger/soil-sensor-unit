// Copyright (c) 2018 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef FW_UPDATE_
#define FW_UPDATE_

#include <cstdint>

#include "etl/vector.h"

#include "bootloader_interface.h"

class FwUpdate {
 public:
  static constexpr int kSectorSize = 1024;
  static constexpr int kMaxImageSize = 12 * 1024;

  explicit FwUpdate(BootloaderInterface& bootloader)
      : bootloader_(bootloader) {}

  bool StartUpdate(uint16_t num_words) {
    if (num_words > kMaxImageSize / 2) {
      return false;
    }

    write_buffer_.clear();
    word_offset_ = 0;
    num_words_ = num_words;

    bootloader_.PrepareUpdate();

    return true;
  }

  bool AddImageData(uint16_t word_offset, uint16_t word) {
    // Check if data is valid. It must be transferred in the right order.
    if (word_offset != word_offset_) {
      return false;
    }

    assert(!write_buffer_.full());
    write_buffer_.push_back(word >> 8);
    write_buffer_.push_back(word & 0xFF);
    word_offset_++;

    if (write_buffer_.full()) {
      bootloader_.WriteImageData((word_offset * 2) - write_buffer_.size(),
                                 write_buffer_.data(), write_buffer_.size());
      write_buffer_.clear();
    }

    // Update finished.
    if (word_offset == num_words_) {
      // Pad remaining bytes with 0xFF.
      if (!write_buffer_.empty()) {
        while (!write_buffer_.full()) {
          write_buffer_.push_back(0xFF);
        }

        bootloader_.WriteImageData(word_offset * 2, write_buffer_.data(),
                                   write_buffer_.size());
      }

      return bootloader_.SetUpdatePending();
    }

    return true;
  }

  bool ConfirmUpdate() { return bootloader_.SetUpdateConfirmed(); }

 private:
  BootloaderInterface& bootloader_;

  etl::vector<uint8_t, kSectorSize> write_buffer_;

  uint16_t word_offset_ = 0;
  uint16_t num_words_ = 0;
};

#endif  // FW_UPDATE_
