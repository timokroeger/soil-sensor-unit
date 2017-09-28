// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

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

// Asigns the stack a slave address and starts operation.
void ModbusStart(uint8_t slave_address);

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
