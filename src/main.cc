// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include <cassert>
#include <cstdint>

#include "chip.h"

#include "config.h"
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

  ModbusData modbus_data;

  modbus::Modbus modbus_stack(modbus_rtu, modbus_data);
  modbus_stack.set_address(247);

  modbus_rtu.Enable();

  // Main loop.
  for (;;) {
    modbus_stack.Execute();

    uint32_t ev = modbus_data.GetEvents();
    if (ev & ModbusData::kResetDevice) {
      // Delay reset by 15ms so that a valid modbus response can be sent.
      // Actual reset is executed in the MRT ISR in isr.c
      Chip_MRT_SetInterval(LPC_MRT_CH2,
                           ((CPU_FREQ / 1000000) * 15000) | MRT_INTVAL_LOAD);
    }
  }
}
