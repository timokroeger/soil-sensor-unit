#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "modbus.h"

using ::testing::_;
using ::testing::StrictMock;

namespace {

class MockModbusData : public ModbusDataInterface {
 public:
  MOCK_METHOD2(ReadRegister, bool(uint16_t address, uint16_t *data_out));
};

class MockModbusHw : public ModbusHwInterface {
 public:
  MOCK_METHOD0(SerialEnable, void());
  MOCK_METHOD2(SerialSend, void(uint8_t *data, int length));
  MOCK_METHOD0(StartTimer, void());
};

class ModbusTest : public ::testing::Test {
 protected:
  ModbusTest() : mock_modbus_data_(), mock_modbus_hw_() {}

  StrictMock<MockModbusData> mock_modbus_data_;
  StrictMock<MockModbusHw> mock_modbus_hw_;
};

TEST_F(ModbusTest, NoInterfaces) {
  ASSERT_DEATH(Modbus modbus(nullptr, nullptr), "");
}

}  // namespace
