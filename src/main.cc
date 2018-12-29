// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include <cassert>
#include <cstdint>

#include "chip.h"

#include "boot/bootloader.h"
#include "config.h"
#include "fw_update.h"
#include "globals.h"
#include "setup.h"
#include "measure.h"
#include "modbus/modbus.h"
#include "modbus_data.h"

// Required by the vendor chip library.
const uint32_t OscRateIn = CPU_FREQ;
const uint32_t ExtRateIn = 0;  // External clock input not used.

static void SystemInit() {
  SystemCoreClockUpdate();

  SetupGpio();
  SetupAdc();
  SetupPwm();
  SetupTimers();
  SetupUart(19200);
  SetupSwichMatrix();
  SetupNVIC();
}

// The switch matrix and system clock (12Mhz by external crystal) were already
// configured by SystemInit() before main was called.
int main() {
  SystemInit();
  MeasureStart();

  // Link global serial interface implementation to protocol.
  modbus::RtuProtocol modbus_rtu(modbus_serial);
  modbus_serial.set_modbus_rtu(&modbus_rtu);

  Bootloader bootloader;
  FwUpdate fw_update(bootloader);
  ModbusData modbus_data(fw_update);

  modbus::Modbus modbus_stack(modbus_rtu, modbus_data);
  modbus_stack.set_address(247);

  modbus_rtu.Enable();

  // Main loop.
  for (;;) {
    modbus_stack.Execute();
  }
}
