#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "modbus.h"

using ::testing::ElementsAreArray;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::_;
namespace {

class MockModbusData : public ModbusDataInterface {
 public:
  MOCK_METHOD2(ReadRegister, bool(uint16_t address, uint16_t *data_out));
  MOCK_METHOD2(WriteRegister, bool(uint16_t address, uint16_t data));
};

class MockModbusHw : public ModbusHwInterface {
 public:
  MOCK_METHOD0(EnableHw, void());
  MOCK_METHOD0(DisableHw, void());
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

  void StartOperationWrongAddr(uint8_t addr) {
    EXPECT_CALL(mock_modbus_hw_, EnableHw()).Times(0);
    modbus_.StartOperation(addr);
  }

  void StartOperation(uint8_t addr) {
    EXPECT_CALL(mock_modbus_hw_, EnableHw());
    EXPECT_CALL(mock_modbus_hw_, StartTimer());
    modbus_.StartOperation(addr);
    modbus_.Timeout(Modbus::kInterCharacterDelay);
    modbus_.Timeout(Modbus::kInterFrameDelay);
  }

  void SendMessage(const uint8_t *data, int length) {
    EXPECT_CALL(mock_modbus_hw_, StartTimer()).Times(length);
    for (int i = 0; i < length; i++) {
      modbus_.ByteStart();
      modbus_.ByteReceived(data[i]);
    }
    modbus_.Timeout(Modbus::kInterCharacterDelay);
    modbus_.Timeout(Modbus::kInterFrameDelay);
  }

  void RequestResponse(const uint8_t *req, size_t req_len, const uint8_t *resp,
                       size_t resp_len) {
    EXPECT_CALL(mock_modbus_hw_, SerialSend(_, _))
        .With(ElementsAreArray(resp, resp_len));
    SendMessage(req, static_cast<int>(req_len));
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
  StartOperationWrongAddr(0);
  for (int i = 248; i <= 255; i++) {
    StartOperationWrongAddr(static_cast<uint8_t>(i));
  }
}

TEST_F(ModbusTest, ValidSlaveAddresses) {
  for (int i = 1; i <= 247; i++) {
    StartOperation(static_cast<uint8_t>(i));

    EXPECT_CALL(mock_modbus_hw_, DisableHw());
    modbus_.StopOperation();
  }
}

TEST_F(ModbusTest, MisbehavingTimer) {
  StartOperation(1);
  Mock::AllowLeak(&mock_modbus_hw_);
  ASSERT_DEATH(modbus_.Timeout(Modbus::kInterFrameDelay), "");
}

TEST_F(ModbusTest, EmptyMessage) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x7E, 0x80,  // CRC
  };

  StartOperation(1);

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

  StartOperation(1);

  RequestResponse(request, sizeof(request), response, sizeof(response));
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

  StartOperation(1);

  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4567, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xABCD), Return(true)));

  RequestResponse(read_register_request, sizeof(read_register_request),
                  read_register_response, sizeof(read_register_response));
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

  StartOperation(1);

  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4567, _))
      .WillOnce(Return(false));

  RequestResponse(read_register_request, sizeof(read_register_request),
                  read_register_response, sizeof(read_register_response));
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

  StartOperation(1);

  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4567, _)).Times(0);

  RequestResponse(read_register_request_0, sizeof(read_register_request_0),
                  read_register_response, sizeof(read_register_response));
  RequestResponse(read_register_request_7E, sizeof(read_register_request_7E),
                  read_register_response, sizeof(read_register_response));
  RequestResponse(read_register_request_100, sizeof(read_register_request_100),
                  read_register_response, sizeof(read_register_response));
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

  StartOperation(1);

  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4567, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xABCD), Return(true)));
  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4568, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xDEAD), Return(true)));
  EXPECT_CALL(mock_modbus_data_, ReadRegister(0x4569, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xBEAF), Return(true)));

  RequestResponse(read_register_request, sizeof(read_register_request),
                  read_register_response, sizeof(read_register_response));
}

TEST_F(ModbusTest, WriteSingleRegister) {
  const uint8_t write_single_register[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB, 0xCD,  // Quantity of Input Registers
      0x93, 0xBC,  // CRC
  };

  StartOperation(1);

  EXPECT_CALL(mock_modbus_data_, WriteRegister(0x4567, 0xABCD)).WillOnce(Return(true));

  RequestResponse(write_single_register, sizeof(write_single_register),
                  write_single_register, sizeof(write_single_register));
}

TEST_F(ModbusTest, WriteSingleRegisterInvalidAddress) {
  const uint8_t write_single_register_request[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB, 0xCD,  // Quantity of Input Registers
      0x93, 0xBC,  // CRC
  };

  const uint8_t write_single_register_response[] = {
      0x01,        // Slave address
      0x86,        // Error code
      0x02,        // Exception code
      0xC3, 0xA1,  // CRC
  };

  StartOperation(1);

  EXPECT_CALL(mock_modbus_data_, WriteRegister(0x4567, 0xABCD))
      .WillOnce(Return(false));

  RequestResponse(
      write_single_register_request, sizeof(write_single_register_request),
      write_single_register_response, sizeof(write_single_register_response));
}

TEST_F(ModbusTest, WriteSingleRegisterInvalidLength) {
  const uint8_t write_single_register_request_too_short[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB,        // Quantity of Input Registers
      0x63, 0x12,  // CRC
  };

  const uint8_t write_single_register_request_too_long[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB, 0xCD,  // Quantity of Input Registers
      0x00,        // Extra Byte
      0xFC, 0x6D,  // CRC
  };

  const uint8_t write_single_register_response[] = {
      0x01,        // Slave address
      0x86,        // Error code
      0x03,        // Exception code
      0x02, 0x61,  // CRC
  };

  StartOperation(1);

  RequestResponse(write_single_register_request_too_short,
                  sizeof(write_single_register_request_too_short),
                  write_single_register_response,
                  sizeof(write_single_register_response));

  RequestResponse(write_single_register_request_too_long,
                  sizeof(write_single_register_request_too_long),
                  write_single_register_response,
                  sizeof(write_single_register_response));
}

TEST_F(ModbusTest, RequestTimeout) {
  // Use a valid request.
  const uint8_t read_register_request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Input Registers
      0x95, 0x19,  // CRC
  };
  const int length = sizeof(read_register_request);

  StartOperation(1);

  EXPECT_CALL(mock_modbus_hw_, StartTimer()).Times(length);

  // But only send half of message…
  for (int i = 0; i < length / 2; i++) {
    modbus_.ByteStart();
    modbus_.ByteReceived(read_register_request[i]);
  }

  // …before there is an timeout.
  modbus_.Timeout(Modbus::kInterCharacterDelay);

  for (int i = length / 2; i < length; i++) {
    modbus_.ByteStart();
    modbus_.ByteReceived(read_register_request[i]);
  }

  // Send rest of message which should be ignored.
  modbus_.Timeout(Modbus::kInterCharacterDelay);
  modbus_.Timeout(Modbus::kInterFrameDelay);
}

TEST_F(ModbusTest, AdditionalRequestBytes) {
  const uint8_t read_register_request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Input Registers
      0x95, 0x19,  // CRC
  };

  StartOperation(1);

  // Send all bytes but last as usual.
  SendMessage(read_register_request, sizeof(read_register_request) - 1);

  EXPECT_CALL(mock_modbus_hw_, StartTimer());
  modbus_.ByteStart();
  modbus_.ByteReceived(
      read_register_request[sizeof(read_register_request) - 1]);
  modbus_.Timeout(Modbus::kInterCharacterDelay);

  // Do not finish frame…
  // modbus_.Timeout(Modbus::kInterFrameDelay);

  // …but send more data.
  SendMessage(read_register_request, sizeof(read_register_request));
}

}  // namespace
