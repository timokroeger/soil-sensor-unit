// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_H_
#define MODBUS_H_

#include <stddef.h>  // size_t
#include <stdint.h>

class ModbusDataInterface;
class ModbusHwInterface;

class Modbus {
 public:
  Modbus(ModbusDataInterface *data_if, ModbusHwInterface *hw_if);

  // Starts operation of the MODBUS stack.
  void StartOperation(uint8_t slave_address);

  // Stops operation of the MODBUS stack.
  void StopOperation();

  // Processes a frame and sends a response or does nothing when no request
  // occurred. Must be called in the main loop to guarantee response times.
  void Update();

  // Notify the MODBUS stack that a new byte was received.
  //
  // Typically called by the UART RX ISR.
  void ByteReceived(uint8_t byte, bool parity_ok);

  // A MODBUS inter frame timeout occurred.
  //
  // The inter-frame delay (IFD) is the minimum time between two frames measured
  // between the end of the stop bit and the beginning of the start bit.
  // IFD = MIN(3.5 * 11 / baudrate, 1750us)
  //
  // Typically called by a timer ISR. The MODBUS stack starts the timer with the
  // ModbusStartTimer() function defined in modbus_callbacks.c.
  void Timeout();

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
    kProcessFrame,
  };

  static const int kBufferSize = 256;

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
  bool frame_valid_;

  uint8_t req_buffer_[kBufferSize];
  uint32_t req_buffer_idx_;

  uint8_t resp_buffer_[kBufferSize];
  uint32_t resp_buffer_idx_;
};

#endif  // MODBUS_H_
