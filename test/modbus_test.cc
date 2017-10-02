#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "modbus.h"

using ::testing::ElementsAreArray;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::_;
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
  ModbusTest()
      : mock_modbus_data_(),
        mock_modbus_hw_(),
        modbus_(&mock_modbus_data_, &mock_modbus_hw_) {}

  StrictMock<MockModbusData> mock_modbus_data_;
  StrictMock<MockModbusHw> mock_modbus_hw_;
  Modbus modbus_;

  void StartOperation() {
    EXPECT_CALL(mock_modbus_hw_, SerialEnable());
    EXPECT_CALL(mock_modbus_hw_, StartTimer());
    modbus_.StartOperation(1);
    modbus_.Timeout(Modbus::kInterCharacterDelay);
    modbus_.Timeout(Modbus::kInterFrameDelay);
  }

  void SendMessage(const uint8_t *data, int length) {
    EXPECT_CALL(mock_modbus_hw_, StartTimer()).Times(length);
    for (int i = 0; i < length; i++) {
      modbus_.ByteReceived(data[i]);
    }
    modbus_.Timeout(Modbus::kInterCharacterDelay);
    modbus_.Timeout(Modbus::kInterFrameDelay);
  }
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
  EXPECT_CALL(mock_modbus_hw_, SerialEnable()).Times(0);
  modbus_.StartOperation(0);

  EXPECT_CALL(mock_modbus_hw_, SerialEnable()).Times(0);
  modbus_.StartOperation(248);

  EXPECT_CALL(mock_modbus_hw_, SerialEnable()).Times(0);
  modbus_.StartOperation(255);
}

TEST_F(ModbusTest, ValidSlaveAddresses) {
  EXPECT_CALL(mock_modbus_hw_, SerialEnable());
  EXPECT_CALL(mock_modbus_hw_, StartTimer());
  modbus_.StartOperation(12);

  EXPECT_CALL(mock_modbus_hw_, SerialEnable());
  EXPECT_CALL(mock_modbus_hw_, StartTimer());
  modbus_.StartOperation(123);

  EXPECT_CALL(mock_modbus_hw_, SerialEnable());
  EXPECT_CALL(mock_modbus_hw_, StartTimer());
  modbus_.StartOperation(247);
}

TEST_F(ModbusTest, EmptyMessage) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x7E, 0x80,  // CRC
  };

  StartOperation();

  EXPECT_CALL(mock_modbus_hw_, SerialSend(_, _)).Times(0);
  SendMessage(request, sizeof(request));
}

TEST_F(ModbusTest, InvalidFunctionCode) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x09,        // Function code
      0xC0, 0x26,  // CRC
  };

  const uint8_t response[] = {
      0x01,        // Slave address
      0x89,        // Error code
      0x01,        // Exception code
      0x86, 0x50,  // CRC
  };

  StartOperation();

  EXPECT_CALL(mock_modbus_hw_, SerialSend(_, _))
      .With(ElementsAreArray(response));
  SendMessage(request, sizeof(request));
}

TEST_F(ModbusTest, ReadInputRegister) {
  const uint8_t read_register_request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Input Registers
      0x95, 0x19,  // CRC
  };

  const uint8_t read_register_response[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x02,        // Byte Count
      0xAB, 0xCD,  // Register Content
      0x07, 0x95,  // CRC
  };

  StartOperation();

  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4567, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xABCD), Return(true)));
  EXPECT_CALL(mock_modbus_hw_, SerialSend(_, _))
      .With(ElementsAreArray(read_register_response));
  SendMessage(read_register_request, sizeof(read_register_request));
}

TEST_F(ModbusTest, ReadInputRegisterInvalidAddress) {
  const uint8_t read_register_request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Input Registers
      0x95, 0x19,  // CRC
  };

  const uint8_t read_register_response[] = {
      0x01,        // Slave address
      0x84,        // Error code
      0x02,        // Exception code
      0xC2, 0xC1,  // CRC
  };

  StartOperation();

  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4567, _))
      .WillOnce(Return(false));
  EXPECT_CALL(mock_modbus_hw_, SerialSend(_, _))
      .With(ElementsAreArray(read_register_response));
  SendMessage(read_register_request, sizeof(read_register_request));
}

TEST_F(ModbusTest, ReadInputRegisterInvalidLength) {
  const uint8_t read_register_request_0[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x00,  // Quantity of Input Registers
      0x54, 0xD9,  // CRC
  };

  const uint8_t read_register_request_7E[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x7E,  // Quantity of Input Registers
      0xD4, 0xF9,  // CRC
  };

  const uint8_t read_register_request_100[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x01, 0x00,  // Quantity of Input Registers
      0x55, 0x49,  // CRC
  };

  const uint8_t read_register_response[] = {
      0x01,        // Slave address
      0x84,        // Error code
      0x03,        // Exception code
      0x03, 0x01,  // CRC
  };

  StartOperation();

  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4567, _)).Times(0);
  EXPECT_CALL(mock_modbus_hw_, SerialSend(_, _))
      .With(ElementsAreArray(read_register_response))
      .Times(3);

  SendMessage(read_register_request_0, sizeof(read_register_request_0));
  SendMessage(read_register_request_7E, sizeof(read_register_request_7E));
  SendMessage(read_register_request_100, sizeof(read_register_request_100));
}

TEST_F(ModbusTest, ReadInputRegisterMultiple) {
  const uint8_t read_register_request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x03,  // Quantity of Input Registers
      0x14, 0xD8,  // CRC
  };

  const uint8_t read_register_response[] = {
      0x01,                                // Slave address
      0x04,                                // Function code
      0x06,                                // Byte Count
      0xAB, 0xCD,                          // Register Content
      0xDE, 0xAD, 0xBE, 0xAF, 0xCE, 0x8D,  // CRC
  };

  StartOperation();

  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4567, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xABCD), Return(true)));
  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4568, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xDEAD), Return(true)));
  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4569, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xBEAF), Return(true)));
  EXPECT_CALL(mock_modbus_hw_, SerialSend(_, _))
      .With(ElementsAreArray(read_register_response));
  SendMessage(read_register_request, sizeof(read_register_request));
}

}  // namespace
