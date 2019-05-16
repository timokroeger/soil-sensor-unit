#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "modbus/rtu_protocol.h"

using ::testing::_;
using ::testing::ElementsAre;
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
    ASSERT_FALSE(rtu_.FrameAvailable());
  }

  void RxFrame(const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
      rtu_.Notify(SerialInterfaceEvents::RxByte{data[i], true});
      EXPECT_EQ(rtu_.FrameAvailable(), false);
    }
    rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  }

  void TxFrame(const uint8_t *data, size_t length) {
    Buffer b(data, data + length);
    rtu_.WriteFrame(&b);
    rtu_.Notify(SerialInterfaceEvents::TxDone{});
  }

  SerialMock serial_;
  RtuProtocol rtu_{serial_};
};

TEST_F(RtuProtocolTest, IdleWithoutData) {
  // The interface implementor wrongly sends a timeout notification despite not
  // receiving any data beforehand.
  rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  ASSERT_FALSE(rtu_.FrameAvailable());
}

TEST_F(RtuProtocolTest, ReceiveFrameTooShort) {
  const uint8_t data[] = {0xFF};
  RxFrame(data, sizeof(data));
  ASSERT_FALSE(rtu_.FrameAvailable());
}

TEST_F(RtuProtocolTest, ReceiveFrameOnlyCrc) {
  const uint8_t data[] = {0xFF, 0xFF};
  RxFrame(data, sizeof(data));
  ASSERT_FALSE(rtu_.FrameAvailable());
}

TEST_F(RtuProtocolTest, ReceiveFrame) {
  const uint8_t data[] = {0x12, 0x3F, 0x4D};

  RxFrame(data, sizeof(data));
  ASSERT_TRUE(rtu_.FrameAvailable());

  Buffer *frame = rtu_.ReadFrame();
  ASSERT_NE(frame, nullptr);
  ASSERT_THAT(*frame, ElementsAre(data[0]));
}

TEST_F(RtuProtocolTest, ReceiveLongFrame) {
  uint8_t data[256] = {0x12, 0x34};
  data[254] = 0x17;
  data[255] = 0x6B;

  RxFrame(data, sizeof(data));
  ASSERT_TRUE(rtu_.FrameAvailable());

  Buffer *frame = rtu_.ReadFrame();
  ASSERT_NE(frame, nullptr);
  ASSERT_THAT(*frame, ElementsAreArray(data, 254));
}

TEST_F(RtuProtocolTest, ReceiveParityError) {
  const uint8_t data[] = {0x12, 0x3F, 0x4D};

  // Parity error in second byte.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data[0], true});
  rtu_.Notify(SerialInterfaceEvents::RxByte{data[1], false});
  rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  ASSERT_FALSE(rtu_.FrameAvailable());
}

TEST_F(RtuProtocolTest, SendFrame) {
  const uint8_t data[] = {0x12, 0x3F, 0x4D};
  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
  TxFrame(data, 1);
}

TEST_F(RtuProtocolTest, SendLongFrame) {
  uint8_t data[256] = {0x12, 0x34};
  data[254] = 0x17;
  data[255] = 0x6B;

  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
  TxFrame(data, 254);
}

TEST_F(RtuProtocolTest, TransmissionSequence) {
  const uint8_t data[] = {0x12, 0x3F, 0x4D};

  RxFrame(data, sizeof(data));
  ASSERT_TRUE(rtu_.FrameAvailable());

  Buffer *frame = rtu_.ReadFrame();
  ASSERT_NE(frame, nullptr);
  ASSERT_THAT(*frame, ElementsAre(data[0]));

  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
  TxFrame(data, 1);

  RxFrame(data, sizeof(data));
  ASSERT_TRUE(rtu_.FrameAvailable());

  frame = rtu_.ReadFrame();
  ASSERT_NE(frame, nullptr);
  ASSERT_THAT(*frame, ElementsAre(data[0]));

  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
  TxFrame(data, 1);
}

TEST_F(RtuProtocolTest, SendDuringReceive) {
  const uint8_t data_recv[] = {0x12, 0x3F, 0x4D};

  // Receive first byte.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data_recv[0], true});
  EXPECT_EQ(rtu_.FrameAvailable(), false);

  // Ignore write operation
  const uint8_t data_send[] = {0x56, 0x3F, 0x7E};
  Buffer b(data_send, data_send + sizeof(data_send));
  EXPECT_CALL(serial_, Send(_, _)).Times(0);
  rtu_.WriteFrame(&b);

  // Receive second byte and third.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data_recv[1], true});
  EXPECT_EQ(rtu_.FrameAvailable(), false);
  rtu_.Notify(SerialInterfaceEvents::RxByte{data_recv[2], true});
  EXPECT_EQ(rtu_.FrameAvailable(), false);

  // Check received data
  rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  ASSERT_TRUE(rtu_.FrameAvailable());

  Buffer *frame = rtu_.ReadFrame();
  ASSERT_NE(frame, nullptr);
  ASSERT_THAT(*frame, ElementsAre(data_recv[0]));
}

TEST_F(RtuProtocolTest, ReceiveDuringSend) {
  const uint8_t data_send[] = {0x12, 0x3F, 0x4D};
  Buffer b(data_send, data_send + 1);
  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data_send));
  rtu_.WriteFrame(&b);

  // Ignore received data.
  const uint8_t data_recv[] = {0x56, 0x3F, 0x7E};
  RxFrame(data_recv, sizeof(data_recv));
  ASSERT_FALSE(rtu_.FrameAvailable());

  // Finish send operation.
  rtu_.Notify(SerialInterfaceEvents::TxDone{});
}

TEST_F(RtuProtocolTest, DisabledBehaviour) {
  // Do some normal stuff to change internal state.
  const uint8_t data[] = {0x12, 0x3F, 0x4D};
  Buffer b(data, data + sizeof(data));
  RxFrame(data, sizeof(data));

  EXPECT_CALL(serial_, Disable());
  rtu_.Disable();

  // Frame received during enabled state is still availble.
  ASSERT_TRUE(rtu_.FrameAvailable());
  rtu_.ReadFrame();

  // No further frames can be read when disabled.
  RxFrame(data, sizeof(data));
  ASSERT_FALSE(rtu_.FrameAvailable());

  // No data can be sent when disabled.
  EXPECT_CALL(serial_, Send(_, _)).Times(0);
  rtu_.WriteFrame(&b);
}

}  // namespace modbus
