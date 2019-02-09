// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include <cassert>
#include <cstdint>

#include "chip.h"

#include "boot/bootloader.h"
#include "globals.h"
#include "setup.h"
#include "measure.h"
#include "modbus/modbus.h"
#include "modbus_data.h"
#include "modbus_data_fw_update.h"

static void Setup() {
  SetupClock();
  SetupGpio();
  SetupAdc();
  SetupPwm();
  SetupTimers();
  SetupUart(19200);
  SetupSwichMatrix();
  SetupNVIC();
}

int main() {
  Setup();

  MeasureStart();

  // Link global serial interface implementation to protocol.
  modbus::RtuProtocol modbus_rtu(modbus_serial);
  modbus_serial.set_modbus_rtu(&modbus_rtu);

  Bootloader bootloader;
  ModbusDataFwUpdate fw_update(bootloader);
  ModbusData modbus_data(fw_update);

  modbus::Modbus modbus_stack(modbus_rtu, modbus_data);
  modbus_stack.set_address(247);

  modbus_rtu.Enable();

  // Main loop.
  for (;;) {
    modbus_stack.Execute();
  }
}
