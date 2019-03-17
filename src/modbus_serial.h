// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_SERIAL_H_
#define MODBUS_SERIAL_H_

#include "stm32g0xx.h"

#include "modbus/rtu_protocol.h"
#include "modbus/serial_interface.h"

class ModbusSerial final : public modbus::SerialInterface {
 public:
  explicit ModbusSerial(USART_TypeDef *usart);

  void Enable() override;
  void Disable() override;
  void Send(const uint8_t *data, size_t length) override;

  void Isr();

  void set_modbus_rtu(modbus::RtuProtocol *modbus_rtu) { rtu_ = modbus_rtu; }
  bool tx_active() const;

 private:
  USART_TypeDef *const usart_;

  modbus::RtuProtocol *rtu_;

  const uint8_t *tx_data_;
  const uint8_t *tx_data_end_;
};

#endif  // MODBUS_SERIAL_H_
