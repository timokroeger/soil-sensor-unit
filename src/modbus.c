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

#include "modbus.h"

#include <stdbool.h>

#include "expect.h"
#include "modbus_callbacks.h"

typedef enum {
  kTransmissionInital = 0,
  kTransmissionIdle,
  kTransmissionReception,
  kTransmissionControlAndWaiting,
} ModbusTransmissionState;

static uint8_t address;

static ModbusTransmissionState transmission_state;
static uint8_t buffer[256];
static uint32_t buffer_idx;
static bool frame_valid;

static uint16_t BufferToWord(const uint8_t *buffer)
{
  return (uint16_t)((buffer[0] << 8) | buffer[1]);
}

// TODO
static void ProcessFrame(void);

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
      frame_valid = (byte == address);

      // Reset buffer and fill first byte.
      buffer[0] = byte;
      buffer_idx = 1;

      transmission_state = kTransmissionReception;
      break;

    case kTransmissionReception:
      ModbusStartTimer();

      // Save data as long as it fits into the buffer.
      if (buffer_idx < sizeof(buffer)) {
        buffer[buffer_idx++] = byte;
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
        if (buffer_idx >= 4) {
          uint16_t received_crc = BufferToWord(&buffer[buffer_idx - 2]);
          uint16_t calculated_crc = ModbusCrc(&buffer[0], buffer_idx - 2);
          if (frame_valid && received_crc == calculated_crc) {
            ProcessFrame();
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
