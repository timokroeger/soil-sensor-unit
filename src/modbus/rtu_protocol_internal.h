// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_RTU_PROTOCOL_INTERNAL_H_
#define MODBUS_RTU_PROTOCOL_INTERNAL_H_

#include "assert.h"

#include "boost/sml.hpp"
#include "etl/crc16_modbus.h"
#include "etl/vector.h"

#include "modbus/serial_interface.h"

namespace modbus {
namespace internal {

namespace sml = boost::sml;

using RtuBuffer = etl::vector<uint8_t, ProtocolInterface::kMaxFrameSize>;

// Events (externel events are inherited from SerialInterfaceEvents)
struct TxStart {
  FrameData data;
};
struct Enable {};
struct Disable {};

struct RtuProtocol : public SerialInterfaceEvents {
  // States
  struct Disabled;
  struct Enabled {
    // States
    struct Init;
    struct Idle;
    struct Receiving;
    struct Ignoring;
    struct Sending;

    auto operator()() const {
      using namespace sml;

      // Guards
      auto parity_ok = [](const RxByte& e) { return e.parity_ok; };
      auto buffer_full = [](const RtuBuffer& b) { return b.full(); };

      // Actions
      auto clear_buffer = [](RtuBuffer& b) { return b.clear(); };
      auto add_byte = [](RtuBuffer& b, const RxByte& e) {
        b.push_back(e.byte);
      };
      auto check_frame = [](RtuBuffer& b, bool& frame_available) {
        if (b.size() > 2) {
          uint16_t crc_recv = b[b.size() - 2] | b[b.size() - 1] << 8;
          uint16_t crc_calc = etl::crc16_modbus(b.begin(), b.end() - 2).value();

          if (crc_recv == crc_calc) {
            // Remove valid CRC from frame data.
            b.pop_back();
            b.pop_back();
            frame_available = true;
          }
        }
      };
      auto invalidate = [](bool& frame_available) { frame_available = false; };
      auto send_frame = [](RtuBuffer& b, const TxStart& txs,
                           SerialInterface& s) {
        assert(txs.data.size() + 2 <= b.capacity());
        uint16_t crc =
            etl::crc16_modbus(txs.data.begin(), txs.data.end()).value();
        b.assign(txs.data.begin(), txs.data.end());
        b.push_back(crc & 0xFF);
        b.push_back(crc >> 8);
        s.Send(b.data(), b.size());
      };

      // clang-format off
      return make_transition_table(
        *state<Init>       + event<BusIdle>                                 = state<Idle>
        ,state<Idle>       + sml::on_exit<_>                 / invalidate

        // Receiving a valid frame
        ,state<Idle>       + event<RxByte> [parity_ok]       / clear_buffer = state<Receiving>
        ,state<Receiving>  + event<RxByte> [parity_ok && !buffer_full]      = state<Receiving>
        ,state<Receiving>  + on_entry<RxByte>                / add_byte
        ,state<Receiving>  + event<BusIdle>                  / check_frame  = state<Idle>

        // Receiving an invalid frame
        ,state<Idle>       + event<RxByte> [!parity_ok]                     = state<Ignoring>
        ,state<Receiving>  + event<RxByte> [!parity_ok || buffer_full]      = state<Ignoring>
        ,state<Ignoring>   + event<BusIdle>                                 = state<Idle>

        // Sending a frame
        ,state<Idle>       + event<TxStart>                  / send_frame   = state<Sending>
        ,state<Sending>    + event<TxDone>                                  = state<Idle>
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
