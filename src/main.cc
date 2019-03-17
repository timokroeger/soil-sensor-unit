// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include <cassert>
#include <cstdint>

#include "stm32g0xx.h"

#include "bootloader.h"
#include "config.h"
#include "globals.h"
#include "setup.h"
#include "measure.h"
#include "modbus/modbus.h"
#include "modbus_data.h"
#include "modbus_data_fw_update.h"

int main() {
  Setup();

  // Link global serial interface implementation to protocol.
  modbus::RtuProtocol modbus_rtu(modbus_serial);
  modbus_serial.set_modbus_rtu(&modbus_rtu);

  Bootloader bootloader;
  ModbusDataFwUpdate fw_update(bootloader);
  ModbusData modbus_data(fw_update);

  modbus::Modbus modbus_stack(modbus_rtu, modbus_data);
  modbus_stack.set_address(CONFIG_SENSOR_ID);

  modbus_rtu.Enable();

  // Main loop.
  for (;;) {
    modbus_stack.Execute();

    if (modbus_data.reset() && !modbus_serial.tx_active()) {
      NVIC_SystemReset();
    }
  }
}
