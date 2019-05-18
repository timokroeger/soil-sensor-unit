#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "modbus/data_interface.h"
#include "modbus/slave.h"

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

namespace modbus {

class DataMock : public DataInterface {
 public:
  MOCK_METHOD2(ReadRegister,
               modbus::ExceptionCode(uint16_t address, uint16_t* data_out));
  MOCK_METHOD2(WriteRegister,
               modbus::ExceptionCode(uint16_t address, uint16_t data));
};

class ModbusTest : public ::testing::Test {
 protected:
  ModbusTest() : data_(), modbus_(data_) {
    ON_CALL(data_, ReadRegister(_, _))
        .WillByDefault(Return(modbus::ExceptionCode::kOk));
    ON_CALL(data_, WriteRegister(_, _))
        .WillByDefault(Return(modbus::ExceptionCode::kOk));
  }

  void SetUp() override { modbus_.set_address(1); }

  void RequestResponse(const uint8_t* req, size_t req_len, const uint8_t* resp,
                       size_t resp_len) {
    const Buffer req_buf{req, req + req_len};
    auto resp_data = modbus_.Execute(&req_buf);
    ASSERT_NE(resp_data, nullptr);
    ASSERT_THAT(*resp_data, ElementsAreArray(resp, resp_len));
  }

  void RequestNoResponse(const uint8_t* req, size_t req_len) {
    const Buffer req_buf{req, req + req_len};
    ASSERT_EQ(modbus_.Execute(&req_buf), nullptr);
  }

  DataMock data_;
  Slave modbus_;
};

TEST_F(ModbusTest, Empty) {
  Buffer req;
  auto resp_data = modbus_.Execute(&req);
  ASSERT_FALSE(resp_data);
}

TEST_F(ModbusTest, ReadInputRegister) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Input Registers
  };

  const uint8_t response[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x02,        // Byte Count
      0xAB, 0xCD,  // Register Content
  };

  EXPECT_CALL(data_, ReadRegister(0x4567, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(0xABCD), Return(modbus::ExceptionCode::kOk)));
  RequestResponse(request, sizeof(request), response, sizeof(response));
}

TEST_F(ModbusTest, ReadInputRegisterMultiple) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x03,  // Quantity of Input Registers
  };

  const uint8_t response[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x06,        // Byte Count
      0xAB, 0xCD,  // Register Content
      0xDE, 0xAD,  // Register Content
      0xBE, 0xAF,  // Register Content
  };

  EXPECT_CALL(data_, ReadRegister(0x4567, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(0xABCD), Return(modbus::ExceptionCode::kOk)));
  EXPECT_CALL(data_, ReadRegister(0x4568, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(0xDEAD), Return(modbus::ExceptionCode::kOk)));
  EXPECT_CALL(data_, ReadRegister(0x4569, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(0xBEAF), Return(modbus::ExceptionCode::kOk)));
  RequestResponse(request, sizeof(request), response, sizeof(response));
}

TEST_F(ModbusTest, ReadInputRegisterMaximum) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x7D,  // Quantity of Input Registers
  };

  const uint8_t response[253] = {
      0x01,  // Slave address
      0x04,  // Function code
      0xFA,  // Byte Count
             // All zeros follow
  };

  EXPECT_CALL(data_, ReadRegister(_, _))
      .Times(125)
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(0x0000), Return(modbus::ExceptionCode::kOk)));
  RequestResponse(request, sizeof(request), response, sizeof(response));
}

TEST_F(ModbusTest, ReadInputRegisterInvalidLength) {
  const uint8_t request1[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x00,  // Quantity of Input Registers
  };

  const uint8_t request2[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x01, 0x00,  // Quantity of Input Registers
  };

  const uint8_t response[] = {
      0x01,  // Slave address
      0x84,  // Error code
      0x03,  // Exception code
  };

  RequestResponse(request1, sizeof(request1), response, sizeof(response));
  RequestResponse(request2, sizeof(request2), response, sizeof(response));
}

TEST_F(ModbusTest, ReadInputRegisterInvalidAddress) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Input Registers
  };

  const uint8_t response[] = {
      0x01,  // Slave address
      0x84,  // Error code
      0x02,  // Exception code
  };

  EXPECT_CALL(data_, ReadRegister(0x4567, _))
      .WillOnce(Return(modbus::ExceptionCode::kIllegalDataAddress));
  RequestResponse(request, sizeof(request), response, sizeof(response));
}

TEST_F(ModbusTest, ReadInputRegisterMalformed) {
  // Byte missing
  const uint8_t request1[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00,        // Quantity of Input Registers
  };

  // Additional byte
  const uint8_t request2[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Input Registers
      0x00,
  };

  const uint8_t response[] = {
      0x01,  // Slave address
      0x84,  // Error code
      0x01,  // Exception code
  };

  RequestNoResponse(request1, sizeof(request1));
  RequestNoResponse(request2, sizeof(request2));
}

TEST_F(ModbusTest, WriteSingleRegister) {
  const uint8_t request_response[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB, 0xCD,  // Register Value
  };

  EXPECT_CALL(data_, WriteRegister(0x4567, 0xABCD))
      .WillOnce(Return(modbus::ExceptionCode::kOk));
  RequestResponse(request_response, sizeof(request_response), request_response,
                  sizeof(request_response));
}

TEST_F(ModbusTest, WriteSingleRegisterInvalidAddress) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB, 0xCD,  // Register Value
  };

  const uint8_t response[] = {
      0x01,  // Slave address
      0x86,  // Error code
      0x02,  // Exception code
  };

  EXPECT_CALL(data_, WriteRegister(0x4567, 0xABCD))
      .WillOnce(Return(modbus::ExceptionCode::kIllegalDataAddress));
  RequestResponse(request, sizeof(request), response, sizeof(response));
}

TEST_F(ModbusTest, WriteSingleRegisterMalformed) {
  const uint8_t request1[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB,        // Register Value
  };

  const uint8_t request2[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB, 0xCD,  // Register Value
      0x00,
  };

  RequestNoResponse(request1, sizeof(request1));
  RequestNoResponse(request2, sizeof(request2));
}

TEST_F(ModbusTest, WriteMultipleRegisters) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x02,  // Quantity of Registers
      0x04,        // Byte Count
      0xDE, 0xAD,  // Register Value 1
      0xBE, 0xEF,  // Register Value 2
  };

  const uint8_t response[] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x02,  // Quantity of Registers
  };

  EXPECT_CALL(data_, WriteRegister(0x4567, 0xDEAD))
      .WillOnce(Return(modbus::ExceptionCode::kOk));
  EXPECT_CALL(data_, WriteRegister(0x4568, 0xBEEF))
      .WillOnce(Return(modbus::ExceptionCode::kOk));
  RequestResponse(request, sizeof(request), response, sizeof(response));
}

TEST_F(ModbusTest, WriteMultipleRegistersMaximum) {
  const uint8_t request[253] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x7B,  // Quantity of Registers
      0xF6,        // Byte Count
                   // Register Values all zero
  };

  const uint8_t response[] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x7B,  // Quantity of Registers
  };

  EXPECT_CALL(data_, WriteRegister(_, 0x0000))
      .Times(123)
      .WillRepeatedly(Return(modbus::ExceptionCode::kOk));
  RequestResponse(request, sizeof(request), response, sizeof(response));
}

TEST_F(ModbusTest, WriteMultipleRegistersInvalidAddress) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x02,  // Quantity of Registers
      0x04,        // Byte Count
      0xDE, 0xAD,  // Register Value 1
      0xBE, 0xEF,  // Register Value 2
  };

  const uint8_t response[] = {
      0x01,  // Slave address
      0x90,  // Error code
      0x02,  // Exception code
  };

  EXPECT_CALL(data_, WriteRegister(0x4567, 0xDEAD))
      .WillOnce(Return(modbus::ExceptionCode::kIllegalDataAddress));
  RequestResponse(request, sizeof(request), response, sizeof(response));
}

TEST_F(ModbusTest, WriteMultipleRegistersInvalidQuantity) {
  const uint8_t request_invalid_quantity1[] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x00,  // Quantity of Registers
      0x00,        // Byte Count
  };

  const uint8_t request_invalid_quantity2[255] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x7C,  // Quantity of Registers
      0xF8,        // Byte Count
                   // Register Values all zero
  };

  const uint8_t request_invalid_bytecount[] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Registers
      0x04,        // Byte Count
      0xAB, 0xCD,  // Registers Value
  };

  const uint8_t response[] = {
      0x01,  // Slave address
      0x90,  // Error code
      0x03,  // Exception code
  };

  RequestResponse(request_invalid_quantity1, sizeof(request_invalid_quantity1),
                  response, sizeof(response));
  RequestResponse(request_invalid_quantity2, sizeof(request_invalid_quantity2),
                  response, sizeof(response));
  RequestResponse(request_invalid_bytecount, sizeof(request_invalid_bytecount),
                  response, sizeof(response));
}

TEST_F(ModbusTest, WriteMultipleRegistersMalformed) {
  const uint8_t request1[] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x02,  // Quantity of Registers
      0x04,        // Byte Count
      0xDE, 0xAD,  // Register Value 1
      0xBE,        // Register Value 2
  };

  const uint8_t request2[] = {
      0x01,        // Slave address
      0x10,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x02,  // Quantity of Registers
      0x04,        // Byte Count
      0xDE, 0xAD,  // Register Value 1
      0xBE, 0xAF,  // Register Value 2
      0x00,
  };

  RequestNoResponse(request1, sizeof(request1));
  RequestNoResponse(request2, sizeof(request2));
}

TEST_F(ModbusTest, MultipleRequests) {
  const uint8_t request[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x45, 0x67,  // Starting Address
      0x00, 0x01,  // Quantity of Input Registers
  };

  const uint8_t response[] = {
      0x01,        // Slave address
      0x04,        // Function code
      0x02,        // Byte Count
      0xAB, 0xCD,  // Register Content
  };

  EXPECT_CALL(data_, ReadRegister(0x4567, _))
      .Times(3)
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(0xABCD), Return(modbus::ExceptionCode::kOk)));
  for (int i = 0; i < 3; i++) {
    RequestResponse(request, sizeof(request), response, sizeof(response));
  }
}

}  // namespace modbus
