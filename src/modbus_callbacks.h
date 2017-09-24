// Copyright (c) 2016 somemetricprefix <somemetricprefix+code@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License

#ifndef MODBUS_CALLBACKS_H_
#define MODBUS_CALLBACKS_H_

#include <stdbool.h>
#include <stdint.h>

// TODO
void ModbusSerialSend(uint8_t *data, int length);

// This function shall start a one-shot timer which calls ModbusTimeout() first
// kModbusTimeoutInterCharacterDelay after typically 750us and then
// kModbusTimeoutInterFrameDelay after typically 1750us.
void ModbusStartTimer(void);

// TODO
uint16_t ModbusCrc(uint8_t *data, uint32_t length);

// Reads the contents of a register at address and writes it to data_out.
// Returns true on success or false when the register is not available.
bool ModbusReadRegister(uint16_t address, uint16_t *data_out);

#endif  // MODBUS_CALLBACKS_H_
