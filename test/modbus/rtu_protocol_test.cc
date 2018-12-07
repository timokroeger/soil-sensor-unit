#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "modbus/rtu_protocol.h"

using ::testing::_;
using ::testing::ElementsAreArray;

namespace modbus {

class SerialMock : public SerialInterface {
 public:
  MOCK_METHOD0(Enable, void());
  MOCK_METHOD0(Disable, void());
  MOCK_METHOD2(Send, void(const uint8_t*, size_t));
};

class RtuProtocolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(serial_, Enable());
    rtu_.Enable();

    rtu_.Notify(SerialInterfaceEvents::BusIdle{});
    ASSERT_EQ(rtu_.FrameAvailable(), false);
  }

  void RxFrame(FrameData data) {
    for (uint8_t b : data) {
      rtu_.Notify(SerialInterfaceEvents::RxByte{b, true});
      EXPECT_EQ(rtu_.FrameAvailable(), false);
    }
    rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  }

  void TxFrame(FrameData data) {
    EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
    rtu_.WriteFrame(data);
    rtu_.Notify(SerialInterfaceEvents::TxDone{});
  }

  SerialMock serial_;
  RtuProtocol rtu_{serial_};
};

TEST_F(RtuProtocolTest, IdleWithoutData) {
  // The interface implementor wrongly sends a timeout notification despite not
  // receiving any data beforehand.
  rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  ASSERT_EQ(rtu_.FrameAvailable(), false);
}

TEST_F(RtuProtocolTest, ReceiveFrame) {
  const uint8_t data[] = {0x12, 0x34};
  RxFrame(FrameData{data});
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), FrameData{data});
}

TEST_F(RtuProtocolTest, ReceiveLongFrame) {
  const uint8_t data[256] = {0x12, 0x34};
  RxFrame(FrameData{data});
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), FrameData{data});
}

TEST_F(RtuProtocolTest, ReceiveParityError) {
  const uint8_t data[] = {0x12, 0x34};

  // Parity error in second byte.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data[0], true});
  rtu_.Notify(SerialInterfaceEvents::RxByte{data[1], false});
  rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  ASSERT_EQ(rtu_.FrameAvailable(), false);
}

TEST_F(RtuProtocolTest, SendFrame) {
  const uint8_t data[] = {0x12, 0x34};
  TxFrame(FrameData{data});
}

TEST_F(RtuProtocolTest, SendLongFrame) {
  const uint8_t data[256] = {0x12, 0x34};
  TxFrame(FrameData{data});
}

TEST_F(RtuProtocolTest, SendTooLongFrame) {
  const uint8_t data[257] = {0x12, 0x34};
  ASSERT_DEATH(rtu_.WriteFrame(FrameData{data}), "");
}

TEST_F(RtuProtocolTest, TransmissionSequence) {
  const uint8_t data[] = {0x12, 0x34};

  RxFrame(FrameData{data});
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), FrameData{data});

  TxFrame(FrameData{data});

  RxFrame(FrameData{data});
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), FrameData{data});

  TxFrame(FrameData{data});
}

TEST_F(RtuProtocolTest, SendDuringReceive) {
  const uint8_t data_recv[] = {0x12, 0x34};

  // Receive first byte.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data_recv[0], true});
  EXPECT_EQ(rtu_.FrameAvailable(), false);

  // Ignore write operation
  const uint8_t data_send[] = {0x56, 0x78};
  EXPECT_CALL(serial_, Send(_, _)).Times(0);
  rtu_.WriteFrame(FrameData{data_send});

  // Receive second byte.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data_recv[1], true});
  EXPECT_EQ(rtu_.FrameAvailable(), false);

  // Check received data
  rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), FrameData{data_recv});
}

TEST_F(RtuProtocolTest, ReceiveDuringSend) {
  const uint8_t data_send[] = {0x56, 0x78};
  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data_send));
  rtu_.WriteFrame(FrameData{data_send});

  // Ignore received data.
  const uint8_t data_recv[] = {0x12, 0x34};
  RxFrame(FrameData{data_recv});
  ASSERT_EQ(rtu_.FrameAvailable(), false);

  // Finishd send operation.
  rtu_.Notify(SerialInterfaceEvents::TxDone{});
}

TEST_F(RtuProtocolTest, DisabledBehaviour) {
  // Do some normal stuff to change internal state.
  const uint8_t data[] = {0x12, 0x34};
  RxFrame(FrameData{data});

  // No frames can be read when disabled.
  rtu_.Disable();
  ASSERT_EQ(rtu_.FrameAvailable(), false);

  // No data can be sent when disabled.
  EXPECT_CALL(serial_, Send(_, _)).Times(0);
  rtu_.WriteFrame(FrameData{data});
}

}  // namespace modbus
