#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "modbus/data_interface.h"
#include "modbus/modbus.h"
#include "modbus/protocol_interface.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

namespace modbus {

class ProtocolMock : public ProtocolInterface {
 public:
  MOCK_METHOD0(FrameAvailable, bool());
  MOCK_METHOD0(ReadFrame, etl::const_array_view<uint8_t>());
  MOCK_METHOD1(WriteFrame, void(etl::const_array_view<uint8_t>));
};

class DataMock : public DataInterface {
 public:
  MOCK_METHOD2(ReadRegister, bool(uint16_t address, uint16_t *data_out));
  MOCK_METHOD2(WriteRegister, bool(uint16_t address, uint16_t data));
};

class ModbusTest : public ::testing::Test {
 protected:
  ModbusTest() : data_(), protocol_(), modbus_(protocol_, data_) {}

  void SetUp() override { modbus_.set_address(1); }

  void RequestResponse(etl::const_array_view<uint8_t> req, etl::const_array_view<uint8_t> resp) {
    InSequence sequence_dummy;
    EXPECT_CALL(protocol_, FrameAvailable()).WillOnce(Return(true));
    EXPECT_CALL(protocol_, ReadFrame()).WillOnce(Return(req));
    EXPECT_CALL(protocol_, WriteFrame(resp));
    modbus_.Execute();
  }

  void RequestNoResponse(etl::const_array_view<uint8_t> req) {
    InSequence sequence_dummy;
    EXPECT_CALL(protocol_, FrameAvailable()).WillOnce(Return(true));
    EXPECT_CALL(protocol_, ReadFrame()).WillOnce(Return(req));
    EXPECT_CALL(protocol_, WriteFrame(_)).Times(0);
    modbus_.Execute();
  }

  ProtocolMock protocol_;
  DataMock data_;
  Modbus modbus_;
};

TEST_F(ModbusTest, FrameNotAvailable) {
  EXPECT_CALL(protocol_, FrameAvailable()).WillOnce(Return(false));
  EXPECT_CALL(protocol_, ReadFrame()).Times(0);
  EXPECT_CALL(protocol_, WriteFrame(_)).Times(0);
  modbus_.Execute();
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
      .WillOnce(DoAll(SetArgPointee<1>(0xABCD), Return(true)));
  RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
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
      .WillOnce(DoAll(SetArgPointee<1>(0xABCD), Return(true)));
  EXPECT_CALL(data_, ReadRegister(0x4568, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xDEAD), Return(true)));
  EXPECT_CALL(data_, ReadRegister(0x4569, _))
      .WillOnce(DoAll(SetArgPointee<1>(0xBEAF), Return(true)));
  RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
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
      .WillRepeatedly(DoAll(SetArgPointee<1>(0x0000), Return(true)));
  RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
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

  RequestResponse(etl::const_array_view<uint8_t>{request1}, etl::const_array_view<uint8_t>{response});
  RequestResponse(etl::const_array_view<uint8_t>{request2}, etl::const_array_view<uint8_t>{response});
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

  EXPECT_CALL(data_, ReadRegister(0x4567, _)).WillOnce(Return(false));
  RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
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

  RequestNoResponse(etl::const_array_view<uint8_t>{request1});
  RequestNoResponse(etl::const_array_view<uint8_t>{request2});
}

TEST_F(ModbusTest, WriteSingleRegister) {
  const uint8_t request_response[] = {
      0x01,        // Slave address
      0x06,        // Function code
      0x45, 0x67,  // Starting Address
      0xAB, 0xCD,  // Register Value
  };

  EXPECT_CALL(data_, WriteRegister(0x4567, 0xABCD)).WillOnce(Return(true));
  RequestResponse(etl::const_array_view<uint8_t>{request_response}, etl::const_array_view<uint8_t>{request_response});
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

  EXPECT_CALL(data_, WriteRegister(0x4567, 0xABCD)).WillOnce(Return(false));
  RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
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

  RequestNoResponse(etl::const_array_view<uint8_t>{request1});
  RequestNoResponse(etl::const_array_view<uint8_t>{request2});
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

  EXPECT_CALL(data_, WriteRegister(0x4567, 0xDEAD)).WillOnce(Return(true));
  EXPECT_CALL(data_, WriteRegister(0x4568, 0xBEEF)).WillOnce(Return(true));
  RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
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
      .WillRepeatedly(Return(true));
  RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
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

  EXPECT_CALL(data_, WriteRegister(0x4567, 0xDEAD)).WillOnce(Return(false));
  RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
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

  RequestResponse(etl::const_array_view<uint8_t>{request_invalid_quantity1}, etl::const_array_view<uint8_t>{response});
  // RequestResponse(etl::const_array_view<uint8_t>{request_invalid_quantity2}, etl::const_array_view<uint8_t>{response});
  // RequestResponse(etl::const_array_view<uint8_t>{request_invalid_bytecount}, etl::const_array_view<uint8_t>{response});
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

  RequestNoResponse(etl::const_array_view<uint8_t>{request1});
  RequestNoResponse(etl::const_array_view<uint8_t>{request2});
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
      .WillRepeatedly(DoAll(SetArgPointee<1>(0xABCD), Return(true)));
  for (int i = 0; i < 3; i++) {
    RequestResponse(etl::const_array_view<uint8_t>{request}, etl::const_array_view<uint8_t>{response});
  }
}

}  // namespace modbus
