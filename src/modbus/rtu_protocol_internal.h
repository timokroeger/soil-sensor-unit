// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_RTU_PROTOCOL_INTERNAL_H_
#define MODBUS_RTU_PROTOCOL_INTERNAL_H_

#include "assert.h"

#include "boost/sml.hpp"
#include "etl/crc16_modbus.h"

#include "modbus.h"
#include "modbus/serial_interface.h"

namespace modbus {
namespace internal {

namespace sml = boost::sml;

// Events
struct Enable {};
struct Disable {};
struct BusIdle {};
struct RxByte {
  uint8_t byte;
  bool parity_ok;
};
struct TxStart {
  Buffer* buf;
};
struct TxDone {};

// State machine
struct RtuProtocol {
  // States
  struct Disabled;
  struct Enabled {
    // Sub-States
    struct Init;
    struct Idle;
    struct Receiving;
    struct Ignoring;
    struct Sending;

    auto operator()() const {
      using namespace sml;

      // Guards
      auto parity_ok = [](const RxByte &e) { return e.parity_ok; };
      auto buffer_full = [](const Buffer& b) { return b.full(); };

      // Actions
      auto clear_buffer = [](Buffer& b, bool& frame_available) {
        b.clear();
        frame_available = false;
      };
      auto add_byte = [](Buffer& b, const RxByte& e) { b.push_back(e.byte); };
      auto check_frame = [](Buffer& b, bool& frame_available) {
        if (b.size() > 2) {
          // Extract CRC from frame data.
          uint16_t crc = b[b.size() - 2] | b[b.size() - 1] << 8;
          b.resize(b.size() - 2);
          frame_available =
              (crc == etl::crc16_modbus(b.begin(), b.end()).value());
        }
      };
      auto send_frame = [](Buffer& b, const TxStart& txs, SerialInterface& s) {
        Buffer* buf = txs.buf;
        assert(buf->capacity() >= buf->size() + 2);

        uint16_t crc = etl::crc16_modbus(buf->begin(), buf->end()).value();
        txs.buf->push_back(crc & 0xFF);
        txs.buf->push_back(crc >> 8);
        s.Send(txs.buf->data(), txs.buf->size());
      };

      // clang-format off
      return make_transition_table(
        *state<Init>       + event<BusIdle>                                                       = state<Idle>

        // Receiving a valid frame
        ,state<Idle>       + event<RxByte> [parity_ok]                 / (clear_buffer, add_byte) = state<Receiving>
        ,state<Receiving>  + event<RxByte> [parity_ok && !buffer_full] / add_byte                 = state<Receiving>
        ,state<Receiving>  + event<BusIdle>                            / check_frame              = state<Idle>

        // Receiving an invalid frame
        ,state<Idle>       + event<RxByte> [!parity_ok]                                           = state<Ignoring>
        ,state<Receiving>  + event<RxByte> [!parity_ok || buffer_full]                            = state<Ignoring>
        ,state<Ignoring>   + event<BusIdle>                                                       = state<Idle>

        // Sending a frame
        ,state<Idle>       + event<TxStart>                            / send_frame               = state<Sending>
        ,state<Sending>    + event<TxDone>                                                        = state<Idle>
      );
      // clang-format on
    }
  };

  auto operator()() const {
    using namespace sml;

    auto enable = [](SerialInterface& hw) { hw.Enable(); };
    auto disable = [](SerialInterface& hw) { hw.Disable(); };

    // clang-format off
    return make_transition_table(
      *state<Disabled> + event<Enable>  / enable  = state<Enabled>
      ,state<Enabled>  + event<Disable> / disable = state<Disabled>
    );
    // clang-format on
  }
};

}  // namespace internal
}  // namespace modbus

#endif  // MODBUS_RTU_PROTOCOL_INTERNAL_H_
