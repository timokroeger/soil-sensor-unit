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
#include <stddef.h>

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
static size_t req_buffer_idx;
static bool frame_valid;

static uint8_t resp_buffer[256];
static size_t resp_buffer_idx;

static const uint8_t crc_lookup_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40 };

static const uint8_t crc_lookup_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
    0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
    0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
    0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
    0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
    0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
    0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
    0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
    0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
    0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
    0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80, 0x40 };

static uint16_t BufferToWordBE(const uint8_t *buffer) {
  return (uint16_t)((buffer[0] << 8) | buffer[1]);
}

static uint16_t BufferToWordLE(const uint8_t *buffer) {
  return (uint16_t)((buffer[1] << 8) | buffer[0]);
}

static void WordToBufferBE(uint16_t word, uint8_t *buffer) {
  buffer[0] = (uint8_t)(word >> 8);
  buffer[1] = (uint8_t)word;
}

static void WordToBufferLE(uint16_t word, uint8_t *buffer) {
  buffer[0] = (uint8_t)word;
  buffer[1] = (uint8_t)(word >> 8);
}

// Taken from Appendix B of "MODBUS over Serial Line Specification and
// Implementation Guide V1.02"
static uint16_t Crc(const uint8_t *data, uint32_t length)
{
  uint8_t crc_hi = 0xFF;
  uint8_t crc_lo = 0xFF;

  while (length--) {
    uint8_t idx = crc_lo ^ *data++;
    crc_lo = crc_hi ^ crc_lookup_hi[idx] ;
    crc_hi = crc_lookup_lo[idx] ;
  }

  return (uint16_t)((crc_hi << 8) | crc_lo);
}

static void ResponseAddByte(uint8_t b) {
  if (resp_buffer_idx + 1 > sizeof(resp_buffer)) {
    return;
  }

  resp_buffer[resp_buffer_idx] = b;
  resp_buffer_idx++;
}

static void ResponseAddWord(uint16_t word) {
  if (resp_buffer_idx + 2 > sizeof(resp_buffer)) {
    return;
  }

  WordToBufferBE(word, &resp_buffer[resp_buffer_idx]);
  resp_buffer_idx += 2;
}

static void SendResponse(void) {
  if (resp_buffer_idx + 2 > sizeof(resp_buffer)) {
    return;
  }

  uint16_t crc = Crc(&resp_buffer[0], resp_buffer_idx);
  WordToBufferLE(crc, &resp_buffer[resp_buffer_idx]);

  ModbusSerialSend(resp_buffer, (int)(resp_buffer_idx + 2));
}

static void SendException(uint8_t exception) {
  resp_buffer[1] |= 0x80;  // Flag response as exception.
  resp_buffer[2] = exception;
  uint16_t crc = Crc(resp_buffer, 3);
  WordToBufferLE(crc, &resp_buffer[3]);
  ModbusSerialSend(resp_buffer, 5);
}

static ModbusException ReadInputRegister(const uint8_t *data, uint32_t length) {
  if (length != 4) {
    return kModbusIllegalDataValue;
  }

  uint16_t starting_addr = BufferToWordBE(&data[0]);
  uint16_t quantity_regs = BufferToWordBE(&data[2]);

  if (quantity_regs < 1 || quantity_regs > 0x7D) {
    return kModbusIllegalDataValue;
  }

  // Byte Count
  ResponseAddByte((uint8_t)(quantity_regs * 2));

  // Add all requested registers to the response.
  for (uint16_t i = 0; i < quantity_regs; i++) {
    uint16_t reg_content = 0;
    bool ok = ModbusReadRegister((uint16_t)(starting_addr + i), &reg_content);
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

  ModbusException exception = kModbusOk;

  uint8_t fn_code = data[0];
  switch (fn_code) {
    case 0x04:
      exception = ReadInputRegister(&data[1], length - 1);
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

void ModbusStart(uint8_t slave_address) {
  address = slave_address;

  // Wait for a inter-frame timeout which then puts the stack in operational
  // (idle) state.
  ModbusStartTimer();
}

void ModbusByteReceived(uint8_t byte) {
  switch (transmission_state) {
    // Ignore received messages until first inter-frame delay is detected.
    case kTransmissionInital:
      ModbusStartTimer();
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
          uint16_t received_crc = BufferToWordLE(&req_buffer[req_buffer_idx - 2]);
          uint16_t calculated_crc = Crc(&req_buffer[0], req_buffer_idx - 2);
          if (frame_valid && received_crc == calculated_crc) {
            // Reset buffer for writing. The first two bytes are the same as in
            // the request.
            resp_buffer[0] = req_buffer[0];  // Address
            resp_buffer[1] = req_buffer[1];  // Function Code
            resp_buffer_idx = 2;

            HandleRequest(&req_buffer[1], req_buffer_idx - 3);
          }
        }

        transmission_state = kTransmissionIdle;
      }
      break;

    default:
      Expect(false);
      break;
  }
}

void ModbusParityError(void) { frame_valid = false; }
