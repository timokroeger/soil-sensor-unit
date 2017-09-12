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

#ifndef MODBUS_H_
#define MODBUS_H_

#include <stdint.h>

// TODO: Think about RX ISR / Timer ISR priority and timing issues.

typedef enum {
  // Inter-character delay is the maximum time between two characters of a frame
  // = MIN(1,5 * 11 / baudrate, 750us)
  kModbusTimeoutInterCharacterDelay,
  // Inter-frame delay is the minimum time between two frames
  // = MIN(3.5 * 11 / baudrate, 1750us)
  kModbusTimeoutInterFrameDelay,
  // Used internally only. Calling ModbusTimeout() with this is a NOOP.
  kModbusTimeoutNone
} ModbusTimeoutType;

// Asigns the stack a slave address.
void ModbusSetAddress(uint8_t address);

// Notify the MODBUS stack that a new byte was received.
//
// Typically called by the UART RX ISR.
void ModbusByteReceived(uint8_t byte);

// A modbus timeout occured.
//
// Typically called by a timer ISR. The MODBUS stack starts the timer with the
// ModbusStartTimer() function defined in modbus_callbacks.c.
void ModbusTimeout(ModbusTimeoutType timeout_type);

// A parity error occured.
//
// Typically called by the UART RX ISR. Make sure to call this function after
// ModbusByteReceived().
void ModbusParityError(void);

#endif  // MODBUS_H_
