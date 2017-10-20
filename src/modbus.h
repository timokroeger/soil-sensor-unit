// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_H_
#define MODBUS_H_

#include <stddef.h>  // size_t
#include <stdint.h>

#include "modbus_data_interface.h"
#include "modbus_hw_interface.h"

// TODO: Think about RX ISR / Timer ISR priority and timing issues.

class Modbus {
 public:
  enum TimeoutType {
    // Inter-character delay is the maximum time between two characters of a
    // frame
    // = MIN(1,5 * 11 / baudrate, 750us)
    kInterCharacterDelay,
    // Inter-frame delay is the minimum time between two frames
    // = MIN(3.5 * 11 / baudrate, 1750us)
    kInterFrameDelay,
    // Used internally only. Calling ModbusTimeout() with this is a NOOP.
    kNoTimeout
  };

  Modbus(ModbusDataInterface *data_if, ModbusHwInterface *hw_if);

  // Starts operation of the MODBUS stack.
  void StartOperation(uint8_t slave_address);

  // Stops operation of the MODBUS stack.
  void StopOperation();

  // Notify the MODBUS stack that a start bit was detected.
  void ByteStart();

  // Notify the MODBUS stack that a new byte was received.
  //
  // Typically called by the UART RX ISR.
  void ByteReceived(uint8_t byte);

  // A MODBUS timeout occurred.
  //
  // Typically called by a timer ISR. The MODBUS stack starts the timer with the
  // ModbusStartTimer() function defined in modbus_callbacks.c.
  void Timeout(TimeoutType timeout_type);

  // A parity error occurred.
  //
  // Typically called by the UART RX ISR. Make sure to call this function after
  // ModbusByteReceived().
  void ParityError() { frame_valid_ = false; }

 private:
  enum ExceptionType {
    kOk = 0,
    kIllegalFunction,
    kIllegalDataAddress,
    kIllegalDataValue,
    kServerDeviceFailure,
    kAcknowledge,
    kDeviceBusy,
    kMemoryParityError,
    kGatewayPathUnavailable,
    kGatewayTargetDeviceFailedToRespond,
  };

  enum TransmissionState {
    kTransmissionInital = 0,
    kTransmissionIdle,
    kTransmissionReception,
    kTransmissionControlAndWaiting,
  };

  void ResponseAddByte(uint8_t b);
  void ResponseAddWord(uint16_t word);
  void SendResponse();
  void SendException(uint8_t exception);

  ExceptionType ReadInputRegister(const uint8_t *data, uint32_t length);
  ExceptionType WriteSingleRegister(const uint8_t *data, uint32_t length);
  void HandleRequest(uint8_t fn_code, const uint8_t *data, uint32_t length);

  ModbusDataInterface *data_interface_;
  ModbusHwInterface *hw_interface_;
  uint8_t address_;

  TransmissionState transmission_state_;
  bool receiving_byte;
  bool frame_valid_;

  uint8_t req_buffer_[256];
  size_t req_buffer_idx_;

  uint8_t resp_buffer_[256];
  size_t resp_buffer_idx_;
};

#endif  // MODBUS_H_
