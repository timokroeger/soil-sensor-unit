// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_RTU_PROTOCOL_INTERNAL_H_
#define MODBUS_RTU_PROTOCOL_INTERNAL_H_

#include "boost/sml.hpp"
#include "etl/vector.h"

#include "modbus/serial_interface.h"

namespace modbus {
namespace internal {

namespace sml = boost::sml;

using RtuBuffer = etl::vector<uint8_t, ProtocolInterface::kMaxFrameSize>;

// Events (externel events are inherited from SerialInterfaceEvents)
struct TxStart {};
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
      auto frame_ok = [](bool& ok) { ok = true; };
      auto frame_nok = [](bool& ok) { ok = false; };
      auto send_frame = [](SerialInterface& s, const RtuBuffer& b) {
        s.Send(b.data(), b.size());
      };

      // clang-format off
      return make_transition_table(
        *state<Init>       + event<BusIdle>                                 = state<Idle>
        ,state<Idle>       + sml::on_exit<_>                 / frame_nok

        // Receiving a valid frame
        ,state<Idle>       + event<RxByte> [parity_ok]       / clear_buffer = state<Receiving>
        ,state<Receiving>  + event<RxByte> [parity_ok]                      = state<Receiving>
        ,state<Receiving>  + on_entry<RxByte> [!buffer_full] / add_byte
        ,state<Receiving>  + event<BusIdle>                  / frame_ok     = state<Idle>

        // Receiving an invalid frame
        ,state<Idle>       + event<RxByte> [!parity_ok]                     = state<Ignoring>
        ,state<Receiving>  + event<RxByte> [!parity_ok]                     = state<Ignoring>
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
