#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "modbus.h"

using ::testing::_;
using ::testing::InSequence;
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

TEST_F(ModbusTest, NoDataInterface) {
  ASSERT_DEATH(Modbus modbus(&mock_modbus_data_, nullptr), "");
}

TEST_F(ModbusTest, NoHwInterface) {
  ASSERT_DEATH(Modbus modbus(nullptr, &mock_modbus_hw_), "");
}

TEST_F(ModbusTest, InvalidSlaveAddresses) {
  Modbus modbus(&mock_modbus_data_, &mock_modbus_hw_);

  modbus.StartOperation(0);
  modbus.StartOperation(248);
  modbus.StartOperation(255);
}

TEST_F(ModbusTest, ValidSlaveAddresses) {
  Modbus modbus(&mock_modbus_data_, &mock_modbus_hw_);

  EXPECT_CALL(mock_modbus_hw_, SerialEnable());
  EXPECT_CALL(mock_modbus_hw_, StartTimer());
  modbus.StartOperation(12);

  EXPECT_CALL(mock_modbus_hw_, SerialEnable());
  EXPECT_CALL(mock_modbus_hw_, StartTimer());
  modbus.StartOperation(123);

  EXPECT_CALL(mock_modbus_hw_, SerialEnable());
  EXPECT_CALL(mock_modbus_hw_, StartTimer());
  modbus.StartOperation(247);
}

}  // namespace
