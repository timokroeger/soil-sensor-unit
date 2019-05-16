// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include <cassert>
#include <cstdint>

#include "bsp/bsp.h"
#include "config.h"
#include "modbus/slave.h"
#include "modbus_data.h"
#include "modbus_data_fw_update.h"

int main() {
  BspSetup();

  modbus_serial.Init(CONFIG_BAUDRATE);

  // Link global serial interface implementation to protocol.
  modbus::RtuProtocol modbus_rtu(modbus_serial);
  modbus_serial.set_modbus_rtu(&modbus_rtu);

  ModbusDataFwUpdate fw_update(bootloader);
  ModbusData modbus_data(fw_update);

  modbus::Slave modbus_slave(modbus_data);
  modbus_slave.set_address(CONFIG_SENSOR_ID);

  modbus_rtu.Enable();

  // Main loop.
  for (;;) {
    if (modbus_rtu.FrameAvailable()) {
      auto req = modbus_rtu.ReadFrame();
      auto resp = modbus_slave.Execute(req);
      if (resp != nullptr) {
        modbus_rtu.WriteFrame(resp);
      }
    }

    if (modbus_data.reset() && !modbus_serial.tx_active()) {
      BspReset();
    }
  }
}
