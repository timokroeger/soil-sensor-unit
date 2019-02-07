#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <numeric>

#include "modbus_data_fw_update.h"

namespace {

class FakeBootloader final : public BootloaderInterface {
 public:
  static constexpr size_t kMemorySize = 4 * 1024;

  bool PrepareUpdate() override {
    prepared = true;
    return true;
  }

  bool WriteImageData(size_t offset, uint8_t* data, size_t length) override {
    if (!prepared) {
      return false;
    }

    auto end = std::copy_n(data, length, update_memory.begin() + offset);
    return end == update_memory.begin() + length;
  }

  bool SetUpdatePending() override {
    pending = true;
    return true;
  }

  bool SetUpdateConfirmed() override { return true; }

  std::array<uint8_t, kMemorySize> update_memory;
  bool prepared = false;
  bool pending = false;
};

TEST(ModbusDataFwUpdateTest, good_update_sequence) {
  std::array<uint8_t, FakeBootloader::kMemorySize> image_data;

  constexpr size_t kActualImageSize = FakeBootloader::kMemorySize - 432;
  std::iota(image_data.begin(), image_data.begin() + kActualImageSize,
            1);  // Fill with dummy data.
  std::fill(image_data.begin() + kActualImageSize, image_data.end(), 0xFF);

  FakeBootloader bl;
  ModbusDataFwUpdate fw_update(bl);

  EXPECT_EQ(fw_update.WriteRegister(ModbusDataFwUpdate::kCommandRegister,
                                    ModbusDataFwUpdate::Command::kPrepare),
            true);
  for (size_t i = 0; i < image_data.size(); i += 2) {
    uint16_t data = image_data[i] << 8 | image_data[i + 1];
    EXPECT_EQ(fw_update.WriteRegister(i / 2, data), true);
  }
  EXPECT_EQ(fw_update.WriteRegister(ModbusDataFwUpdate::kCommandRegister,
                                    ModbusDataFwUpdate::Command::kSetPending),
            true);

  EXPECT_EQ(bl.prepared, true);
  EXPECT_EQ(bl.update_memory, image_data);
  EXPECT_EQ(bl.pending, true);
}

}  // namespace
