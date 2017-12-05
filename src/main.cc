// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include <stdint.h>

#include "chip.h"

#include "common.h"
#include "config_storage.h"
#include "expect.h"
#include "globals.h"
#include "setup.h"
#include "measure.h"
#include "modbus.h"
#include "modbus_data.h"

extern "C" {

// Required by the vendor chip library.
const uint32_t OscRateIn = OSC_FREQ;
const uint32_t ExtRateIn = 0;  // External clock input not used.

// Called by asm setup (startup_LPC82x.s) code before main() is executed.
// Sets up the switch matrix (peripheral to pin connections) and the system
// clock.
void SystemInit() {
  //SetupMainClockCrystal();
  SetupSwichMatrix();
  SetupAdc();
  SetupPwm();
  SetupTimers();
  SetupUart(ConfigStorage::Instance().Get(ConfigStorage::kBaudrate));
  SetupNVIC();
}

// The switch matrix and system clock (12Mhz by external crystal) were already
// configured by SystemInit() before main was called.
int main() {
  MeasureStart();

  uint32_t sensor_id = ConfigStorage::Instance().Get(ConfigStorage::kSlaveId);
  Expect(sensor_id >= 1 && sensor_id <= 247);
  modbus.StartOperation(static_cast<uint8_t>(sensor_id));

  // Main loop.
  for (;;) {
    modbus.Update();

    uint32_t ev = modbus_data.GetEvents();
    if (ev & ModbusData::kWriteConfiguration) {
      ConfigStorage::Instance().WriteConfigToFlash();
    }

    if (ev & ModbusData::kResetDevice) {
      NVIC_SystemReset();
      Expect(false);
    }
  }

  // Never reached.
  return 0;
}

}
