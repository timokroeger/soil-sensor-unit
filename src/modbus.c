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

// TODO: What do when multiple requests?

#include "modbus.h"

#include <stdbool.h>

#include "expect.h"
#include "modbus_callbacks.h"

typedef enum {
  kModbusOk = 0,
  kModbusIllegalFunction,
  kModbusIllegalDataAddress,
  kModbusIllegalDataValue,
  kModbusServerDeviceFailure,
  kModbusAcknowledge,
  kModbusDeviceBusy,
  kModbusMemoryParityError,
  kModbusGatewayPathUnavailable,
  kModbusGatewayTargetDeviceFailedToRespond,
} ModbusException;

typedef enum {
  kTransmissionInital = 0,
  kTransmissionIdle,
  kTransmissionReception,
  kTransmissionControlAndWaiting,
} ModbusTransmissionState;

static uint8_t address;

static ModbusTransmissionState transmission_state;
static uint8_t req_buffer[256];
static uint32_t req_buffer_idx;
static bool frame_valid;

static uint16_t BufferToWord(const uint8_t *buffer) {
  return (uint16_t)((buffer[0] << 8) | buffer[1]);
}

static ModbusException ReadInputRegister(const uint8_t *data, uint32_t length) {
  if (length != 4) {
    return kModbusIllegalDataValue;
  }

  uint16_t starting_addr = BufferToWord(&data[0]);
  uint16_t quantity_regs = BufferToWord(&data[2]);

  if (quantity_regs >= 1 && quantity_regs <= 0x7D) {
    return kModbusIllegalDataValue;
  }

  for (uint16_t i = 0; i < quantity_regs; i++) {
    uint16_t reg_content = 0;
    bool ok = ModbusReadRegister(starting_addr + i, &reg_content);
    if (ok) {
      ResponseAddWord(reg_content);
    } else {
      return kModbusIllegalDataAddress;
    }
  }

  return kModbusOk;
}

static void HandleRequest(const uint8_t *data, uint32_t length) {
  if (length == 0) {
    SendException(kModbusIllegalFunction);
    return;
  }

  bool exception = kModbusOk;

  uint8_t fn_code = data[0];
  switch (fn_code) {
    case 0x04:
      exception = ReadInputRegister(&data[1], length)
                      ? kModbusOk
                      : kModbusIllegalDataAddress;
      break;

    default:
      exception = kModbusIllegalFunction;
      break;
  }

  if (exception == kModbusOk) {
    SendResponse();
  } else {
    SendException(exception);
  }
}

void ModbusSetAddress(uint8_t addr) { address = addr; }

void ModbusByteReceived(uint8_t byte) {
  switch (transmission_state) {
    // Ignore received messages until first inter-frame delay is detected.
    case kTransmissionInital:
      break;

    // First byte: Start of frame
    case kTransmissionIdle:
      ModbusStartTimer();

      // Immediately check if address matches.
      // TODO: Allow broadcasts.
      frame_valid = (byte == address);

      // Reset buffer and fill first byte.
      req_buffer[0] = byte;
      req_buffer_idx = 1;

      transmission_state = kTransmissionReception;
      break;

    case kTransmissionReception:
      ModbusStartTimer();

      // Save data as long as it fits into the buffer.
      if (req_buffer_idx < sizeof(req_buffer)) {
        req_buffer[req_buffer_idx++] = byte;
      }

      break;

    case kTransmissionControlAndWaiting:
      frame_valid = false;
      break;

    default:
      Expect(false);
      break;
  }
}

void ModbusTimeout(ModbusTimeoutType timeout_type) {
  switch (transmission_state) {
    case kTransmissionInital:
      if (timeout_type == kModbusTimeoutInterFrameDelay) {
        transmission_state = kTransmissionIdle;
      }
      break;

    case kTransmissionIdle:
      // No timeouts during idling.
      break;

    case kTransmissionReception:
      if (timeout_type == kModbusTimeoutInterCharacterDelay) {
        transmission_state = kTransmissionControlAndWaiting;
      }

    case kTransmissionControlAndWaiting:
      if (timeout_type == kModbusTimeoutInterFrameDelay) {
        // Minimum message size is: 4b (= 1b addr + 1b fn_code + 2b CRC)
        if (req_buffer_idx >= 3) {
          uint16_t received_crc = BufferToWord(&req_buffer[req_buffer_idx - 2]);
          uint16_t calculated_crc = ModbusCrc(&req_buffer[0], req_buffer_idx - 2);
          if (frame_valid && received_crc == calculated_crc) {
            HandleRequest(&req_buffer[1], req_buffer_idx - 3);
          }
        }

        transmission_state = kTransmissionIdle;
      }

    default:
      Expect(false);
      break;
  }
}

void ModbusParityError(void) { frame_valid = false; }
