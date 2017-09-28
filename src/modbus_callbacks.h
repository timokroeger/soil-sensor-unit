// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_CALLBACKS_H_
#define MODBUS_CALLBACKS_H_

#include <stdbool.h>
#include <stdint.h>

// Initalises the serial interface and the timers required by the modbus stack.
//
// The user must configure the serial interface with the desired baudrate,
// parity and stop bits. As inter-character and inter-frame timeout values
// depend on baudrate it is also up to the user to decide on those.
void ModbusInitHw(void);

// Sends a modbus response via serial interface.
void ModbusSerialSend(uint8_t *data, int length);

// This function shall start a one-shot timer which calls ModbusTimeout() first
// kModbusTimeoutInterCharacterDelay after typically 750us and then
// kModbusTimeoutInterFrameDelay after typically 1750us.
void ModbusStartTimer(void);

// Reads the contents of a register at address and writes it to data_out.
// Returns true on success or false when the register is not available.
bool ModbusReadRegister(uint16_t address, uint16_t *data_out);

#endif  // MODBUS_CALLBACKS_H_
