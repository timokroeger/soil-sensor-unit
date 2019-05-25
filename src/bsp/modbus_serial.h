// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef BSP_MODBUS_SERIAL_H_
#define BSP_MODBUS_SERIAL_H_

#include "chip.h"

#include "modbus/rtu_protocol.h"
#include "modbus/serial_interface.h"

class ModbusSerial final : public modbus::SerialInterface {
 public:
  ModbusSerial(LPC_USART_T *usart, LPC_MRT_CH_T *mrt_ch)
      : usart_(usart), mrt_ch_(mrt_ch) {}

  void Init(uint32_t baudrate);

  void Enable();
  void Disable();

  void Send(const uint8_t *data, size_t length) override;

  void TimerIsr();
  void UartIsr();

  void set_modbus_rtu(modbus::RtuProtocol *modbus_rtu) { rtu_ = modbus_rtu; }
  bool tx_active() const { return tx_active_; }

 private:
  LPC_USART_T *const usart_;
  LPC_MRT_CH_T *const mrt_ch_;

  modbus::RtuProtocol *rtu_ = nullptr;

  volatile bool tx_active_ = false;
  const uint8_t *tx_data_ = nullptr;
  const uint8_t *tx_data_end_ = nullptr;
};

#endif  // BSP_MODBUS_SERIAL_H_
