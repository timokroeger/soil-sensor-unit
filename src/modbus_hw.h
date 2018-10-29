// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_HW_H_
#define MODBUS_HW_H_

#include <stdint.h>

#include "modbus_hw_interface.h"

class ModbusHw : public ModbusHwInterface {
 public:
  void EnableHw() override;
  void DisableHw() override;
  void SerialSend(uint8_t *data, int length) override;
};

#endif  // MODBUS_HW_H_
