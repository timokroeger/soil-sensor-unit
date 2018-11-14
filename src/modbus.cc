// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus.h"

#include <assert.h>

#include "etl/crc16_modbus.h"
#include "modbus_data_interface.h"
#include "modbus_hw_interface.h"

static uint16_t BufferToWordBE(const uint8_t *buffer) {
  return static_cast<uint16_t>((buffer[0] << 8) | buffer[1]);
}

static uint16_t BufferToWordLE(const uint8_t *buffer) {
  return static_cast<uint16_t>((buffer[1] << 8) | buffer[0]);
}

static uint16_t Crc(const uint8_t *data, size_t length) {
  return static_cast<uint16_t>(etl::crc16_modbus(data, data + length).value());
}

Modbus::Modbus(ModbusDataInterface *data_if, ModbusHwInterface *hw_if)
    : transmission_state_(kTransmissionInital),
      data_interface_(data_if),
      hw_interface_(hw_if),
      address_(0) {
  assert(data_interface_ != nullptr);
  assert(hw_interface_ != nullptr);
}

void Modbus::ResponseAddByte(uint8_t b) { resp_buffer_.push_back(b); }

void Modbus::ResponseAddWord(uint16_t word) {
  resp_buffer_.push_back(static_cast<uint8_t>(word >> 8));
  resp_buffer_.push_back(static_cast<uint8_t>(word));
}

void Modbus::ResponseAddCrc() {
  uint16_t crc = Crc(resp_buffer_.data(), resp_buffer_.size());
  resp_buffer_.push_back(static_cast<uint8_t>(crc));
  resp_buffer_.push_back(static_cast<uint8_t>(crc >> 8));
}

void Modbus::SendException(uint8_t exception) {
  resp_buffer_.clear();
  ResponseAddByte(req_buffer_[0]);         // Copy address from request.
  ResponseAddByte(req_buffer_[1] | 0x80);  // Flag response as exception.
  ResponseAddByte(exception);
  ResponseAddCrc();
  hw_interface_->SerialSend(resp_buffer_.data(), resp_buffer_.size());
}

Modbus::ExceptionType Modbus::ReadInputRegister(const uint8_t *data,
                                                size_t length) {
  if (length != 4) {
    return kIllegalDataValue;
  }

  uint16_t starting_addr = BufferToWordBE(&data[0]);
  uint16_t quantity_regs = BufferToWordBE(&data[2]);

  if (quantity_regs < 1 || quantity_regs > 0x7D) {
    return kIllegalDataValue;
  }

  // Byte Count
  ResponseAddByte(static_cast<uint8_t>(quantity_regs * 2));

  // Add all requested registers to the response.
  for (int i = 0; i < quantity_regs; i++) {
    uint16_t reg_content = 0;
    uint16_t addr = static_cast<uint16_t>(starting_addr + i);
    bool ok = data_interface_->ReadRegister(addr, &reg_content);
    if (ok) {
      ResponseAddWord(reg_content);
    } else {
      return kIllegalDataAddress;
    }
  }

  return kOk;
}

Modbus::ExceptionType Modbus::WriteSingleRegister(const uint8_t *data,
                                                  size_t length) {
  if (length != 4) {
    return kIllegalDataValue;
  }

  uint16_t wr_addr = BufferToWordBE(&data[0]);
  uint16_t wr_data = BufferToWordBE(&data[2]);

  bool ok = data_interface_->WriteRegister(wr_addr, wr_data);
  if (ok) {
    ResponseAddWord(wr_addr);
    ResponseAddWord(wr_data);
    return kOk;
  } else {
    return kIllegalDataAddress;
  }
}

void Modbus::HandleRequest(uint8_t fn_code, const uint8_t *data,
                           size_t length) {
  ExceptionType exception = kOk;

  switch (fn_code) {
    case 0x04:
      exception = ReadInputRegister(data, length);
      break;

    case 0x06:
      exception = WriteSingleRegister(data, length);
      break;

    default:
      exception = kIllegalFunction;
      break;
  }

  if (exception == kOk) {
    ResponseAddCrc();
    hw_interface_->SerialSend(resp_buffer_.data(), resp_buffer_.size());
  } else {
    SendException(exception);
  }
}

void Modbus::StartOperation(uint8_t slave_address) {
  assert(transmission_state_ == kTransmissionInital);
  if (slave_address >= 1 && slave_address <= 247) {
    address_ = slave_address;

    hw_interface_->EnableHw();
  }
}

void Modbus::StopOperation() {
  hw_interface_->DisableHw();
  transmission_state_ = kTransmissionInital;
}

void Modbus::Update() {
  if (transmission_state_ == kProcessingFrame) {
    // Minimum message size is: 4b (= 1b addr + 1b fn_code + 2b CRC)
    // TODO: Allow broadcasts (addr = 0)
    if ((req_buffer_.size() >= 4) && (req_buffer_[0] == address_)) {
      // Extract CRC from message and calculate our own.
      uint16_t received_crc =
          BufferToWordLE(&req_buffer_[req_buffer_.size() - 2]);
      uint16_t calculated_crc = Crc(req_buffer_.data(), req_buffer_.size() - 2);
      if (received_crc == calculated_crc) {
        resp_buffer_.clear();

        // The first two bytes are the same as in the request.
        ResponseAddByte(req_buffer_[0]);  // Address
        ResponseAddByte(req_buffer_[1]);  // Function code

        HandleRequest(req_buffer_[1], &req_buffer_[2], req_buffer_.size() - 4);
      }
    }

    transmission_state_ = kTransmissionIdle;
  }
}

void Modbus::ByteReceived(uint8_t byte, bool parity_ok) {
  if (!parity_ok) {
    transmission_state_ = kInvalidFrame;
  }

  switch (transmission_state_) {
    // Ignore received messages until first inter-frame delay is detected.
    case kTransmissionInital:
      break;

    // First byte: Start of frame
    case kTransmissionIdle:
      // Reset buffer and fill first byte.
      req_buffer_.clear();
      req_buffer_.push_back(byte);

      transmission_state_ = kTransmissionReception;
      break;

    case kTransmissionReception:
      // Save data as long as it fits into the buffer.
      if (!req_buffer_.full()) {
        req_buffer_.push_back(byte);
      }
      break;

    case kProcessingFrame:
      // There should be no more bytes after a request frame. If there are some
      // ignore them.
      break;

    case kInvalidFrame:
      // Silently discard data of invalid frames.
      break;

    default:
      assert(false);
      break;
  }
}

void Modbus::Timeout() {
  switch (transmission_state_) {
    case kTransmissionInital:
      transmission_state_ = kTransmissionIdle;
      break;

    case kTransmissionIdle:
      assert(false);
      break;

    case kTransmissionReception:
      transmission_state_ = kProcessingFrame;
      break;

    case kProcessingFrame:
      // Just as there should be no data there should also be no inter-frame
      // delay timeout. Ignore timeout if it occurres anyway.
      break;

    case kInvalidFrame:
      // Frame finished try to receive the next one.
      transmission_state_ = kTransmissionIdle;
      break;

    default:
      assert(false);
      break;
  }
}
