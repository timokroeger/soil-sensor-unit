// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_DATA_H_
#define MODBUS_DATA_H_

#include "modbus/data_interface.h"

class ModbusData final : public modbus::DataInterface {
 public:
  ModbusData(modbus::DataInterface &fw_update) : fw_update_(fw_update) {}

  modbus::ExceptionCode ReadRegister(uint16_t address,
                                     uint16_t *data_out) override;
  modbus::ExceptionCode WriteRegister(uint16_t address, uint16_t data) override;

  bool reset() const { return reset_; }

 private:
  modbus::DataInterface &fw_update_;
  bool reset_ = false;
};

#endif  // MODBUS_DATA_H_
