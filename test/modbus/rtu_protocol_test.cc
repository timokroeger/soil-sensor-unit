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

  void RxFrame(etl::const_array_view<uint8_t> data) {
    for (uint8_t b : data) {
      rtu_.Notify(SerialInterfaceEvents::RxByte{b, true});
      EXPECT_EQ(rtu_.FrameAvailable(), false);
    }
    rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  }

  void TxFrame(etl::const_array_view<uint8_t> data) {
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

TEST_F(RtuProtocolTest, ReceiveFrameTooShort) {
  const uint8_t data[] = {0xFF};
  RxFrame(etl::const_array_view<uint8_t>{data});
  ASSERT_EQ(rtu_.FrameAvailable(), false);
}

TEST_F(RtuProtocolTest, ReceiveFrameOnlyCrc) {
  const uint8_t data[] = {0xFF, 0xFF};
  RxFrame(etl::const_array_view<uint8_t>{data});
  ASSERT_EQ(rtu_.FrameAvailable(), false);
}

TEST_F(RtuProtocolTest, ReceiveFrame) {
  const uint8_t data[] = {0x12, 0x3F, 0x4D};
  RxFrame(etl::const_array_view<uint8_t>{data});
  etl::const_array_view<uint8_t> out(data, 1);
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), out);
}

TEST_F(RtuProtocolTest, ReceiveLongFrame) {
  uint8_t data[256] = {0x12, 0x34};
  data[254] = 0x17;
  data[255] = 0x6B;

  RxFrame(etl::array_view<uint8_t>{data});
  etl::array_view<uint8_t> out(data, 254);
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), out);
}

TEST_F(RtuProtocolTest, ReceiveParityError) {
  const uint8_t data[] = {0x12, 0x3F, 0x4D};

  // Parity error in second byte.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data[0], true});
  rtu_.Notify(SerialInterfaceEvents::RxByte{data[1], false});
  rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  ASSERT_EQ(rtu_.FrameAvailable(), false);
}

TEST_F(RtuProtocolTest, SendFrame) {
  const uint8_t data[] = {0x12, 0x3F, 0x4D};
  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
  TxFrame(etl::const_array_view<uint8_t>{data, 1});
}

TEST_F(RtuProtocolTest, SendLongFrame) {
  uint8_t data[256] = {0x12, 0x34};
  data[254] = 0x17;
  data[255] = 0x6B;

  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
  TxFrame(etl::array_view<uint8_t>{data, 254});
}

TEST_F(RtuProtocolTest, SendTooLongFrame) {
  const uint8_t data[257] = {0x12, 0x34};
  ASSERT_DEATH(rtu_.WriteFrame(etl::const_array_view<uint8_t>{data}), "");
}

TEST_F(RtuProtocolTest, TransmissionSequence) {
  const uint8_t data[] = {0x12, 0x3F, 0x4D};
  etl::const_array_view<uint8_t> data_recv{data, 1};

  RxFrame(etl::const_array_view<uint8_t>{data});
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), data_recv);

  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
  TxFrame(etl::const_array_view<uint8_t>{data, 1});

  RxFrame(etl::const_array_view<uint8_t>{data});
  ASSERT_EQ(rtu_.FrameAvailable(), true);
  ASSERT_EQ(rtu_.ReadFrame(), data_recv);

  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data));
  TxFrame(etl::const_array_view<uint8_t>{data, 1});
}

TEST_F(RtuProtocolTest, SendDuringReceive) {
  const uint8_t data_recv[] = {0x12, 0x3F, 0x4D};

  // Receive first byte.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data_recv[0], true});
  EXPECT_EQ(rtu_.FrameAvailable(), false);

  // Ignore write operation
  const uint8_t data_send[] = {0x56, 0x3F, 0x7E};
  EXPECT_CALL(serial_, Send(_, _)).Times(0);
  rtu_.WriteFrame(etl::const_array_view<uint8_t>{data_send, 1});

  // Receive second byte and third.
  rtu_.Notify(SerialInterfaceEvents::RxByte{data_recv[1], true});
  EXPECT_EQ(rtu_.FrameAvailable(), false);
  rtu_.Notify(SerialInterfaceEvents::RxByte{data_recv[2], true});
  EXPECT_EQ(rtu_.FrameAvailable(), false);

  // Check received data
  rtu_.Notify(SerialInterfaceEvents::BusIdle{});
  ASSERT_EQ(rtu_.FrameAvailable(), true);

  etl::const_array_view<uint8_t> out(data_recv, 1);
  ASSERT_EQ(rtu_.ReadFrame(), out);
}

TEST_F(RtuProtocolTest, ReceiveDuringSend) {
  const uint8_t data_send[] = {0x12, 0x3F, 0x4D};
  EXPECT_CALL(serial_, Send(_, _)).With(ElementsAreArray(data_send));
  rtu_.WriteFrame(etl::const_array_view<uint8_t>{data_send, 1});

  // Ignore received data.
  const uint8_t data_recv[] = {0x56, 0x3F, 0x7E};
  RxFrame(etl::const_array_view<uint8_t>{data_recv});
  ASSERT_EQ(rtu_.FrameAvailable(), false);

  // Finish send operation.
  rtu_.Notify(SerialInterfaceEvents::TxDone{});
}

TEST_F(RtuProtocolTest, DisabledBehaviour) {
  // Do some normal stuff to change internal state.
  const uint8_t data[] = {0x12, 0x3F, 0x4D};
  RxFrame(etl::const_array_view<uint8_t>{data});

  // No frames can be read when disabled.
  rtu_.Disable();
  ASSERT_EQ(rtu_.FrameAvailable(), false);

  // No data can be sent when disabled.
  EXPECT_CALL(serial_, Send(_, _)).Times(0);
  rtu_.WriteFrame(etl::const_array_view<uint8_t>{data});
}

}  // namespace modbus
