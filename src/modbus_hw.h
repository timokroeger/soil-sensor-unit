// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_HW_H_
#define MODBUS_HW_H_

#include "modbus_hw_interface.h"

class ModbusHw : public ModbusHwInterface {
 public:
  ModbusHw();

  void ModbusSerialEnable() override;
  void ModbusSerialSend(uint8_t *data, int length) override;
  void ModbusStartTimer() override;

 private:
  // Sets up UART0 with RTS pin as drive enable for the RS485 receiver.
  static void SetupUart();
  static void SetupTimers();
};

#endif  // MODBUS_HW_H_
