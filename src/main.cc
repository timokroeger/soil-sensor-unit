// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include <cassert>
#include <cstdint>

#include "bsp/bsp.h"
#include "config.h"
#include "modbus/slave.h"
#include "modbus_data.h"
#include "modbus_data_fw_update.h"

namespace {

void UpdateModbus(modbus::RtuProtocol &rtu, modbus::Slave &slave) {
  BspInterruptFree _;

  auto req = rtu.ReadFrame();
  if (req == nullptr) {
    return;
  }

  modbus::Buffer resp;
  bool ok = slave.Execute(req.get(), &resp);
  if (!ok) {
    return;
  }

  rtu.WriteFrame(&resp);
}

}  // namespace

int main() {
  BspSetup();

  modbus_serial.Init(CONFIG_BAUDRATE);

  // Link global serial interface implementation to protocol.
  modbus::RtuProtocol modbus_rtu(modbus_serial);
  modbus_serial.set_modbus_rtu(&modbus_rtu);
  modbus_serial.Enable();

  ModbusDataFwUpdate fw_update(bootloader);
  ModbusData modbus_data(fw_update);

  modbus::Slave modbus_slave(modbus_data);
  modbus_slave.set_address(CONFIG_SENSOR_ID);

  // Main loop.
  for (;;) {
    UpdateModbus(modbus_rtu, modbus_slave);

    if (modbus_data.reset() && !modbus_serial.tx_active()) {
      BspReset();
    }

    BspSleep();
  }
}
