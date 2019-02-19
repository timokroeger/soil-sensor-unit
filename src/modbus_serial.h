// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_SERIAL_H_
#define MODBUS_SERIAL_H_

#include "modbus/rtu_protocol.h"
#include "modbus/serial_interface.h"

class ModbusSerial final : public modbus::SerialInterface {
 public:
  void Enable() override;
  void Disable() override;
  void Send(const uint8_t *data, size_t length) override;

  void TimerIsr();
  void UartIsr();

  void set_modbus_rtu(modbus::RtuProtocol *modbus_rtu) { rtu_ = modbus_rtu; }
  bool tx_active() const { return tx_active_; }

 private:
  modbus::RtuProtocol *rtu_;

  bool tx_active_;
  const uint8_t *tx_data_;
  const uint8_t *tx_data_end_;
};

#endif  // MODBUS_SERIAL_H_
