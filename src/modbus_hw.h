// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_HW_H_
#define MODBUS_HW_H_

#include "modbus.h"
#include "modbus_hw_interface.h"

class ModbusHw : public ModbusHwInterface {
 public:
  ModbusHw();

  void EnableHw() override;
  void DisableHw() override;
  void SerialSend(uint8_t *data, int length) override;
  void StartTimer() override;

  static Modbus *modbus() { return modbus_; }
  static void set_modbus(Modbus *modbus) { modbus_ = modbus; }

 private:
  // Sets up UART0 with RTS pin as drive enable for the RS485 receiver.
  static void SetupUart();
  static void SetupTimers();

  static Modbus *modbus_;
};

#endif  // MODBUS_HW_H_
