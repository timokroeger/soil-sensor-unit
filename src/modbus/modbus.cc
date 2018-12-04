#include "modbus/modbus.h"

namespace modbus {

static uint16_t BufferToWordBE(const uint8_t *buffer) {
  return static_cast<uint16_t>((buffer[0] << 8) | buffer[1]);
}

bool Modbus::Execute() {
  if (!protocol_.FrameAvailable()) {
    return false;
  }

  FrameData fd = protocol_.ReadFrame();
  if (fd.size() < 2) {
    return false;
  }

  // TODO: Allow broadcasts (addr = 0)
  uint8_t addr = fd[0];
  if (addr != address_) {
    return false;
  }
  ResponseAddByte(addr);

  uint8_t fn_code = fd[1];
  ResponseAddByte(fn_code);

  // Discard adress and function code from data.
  FrameData data(&fd[2], fd.size() - 2);

  ExceptionCode exception = ExceptionCode::kOk;
  switch (static_cast<FunctionCode>(fn_code)) {
    case FunctionCode::kReadInputRegister:
      exception = ReadInputRegister(data);
      break;

    case FunctionCode::kWriteSingleRegister:
      exception = WriteSingleRegister(data);
      break;

    case FunctionCode::kWriteMultipleRegisters:
      // TODO
      break;

    default:
      // Function code is not supported: Reply with an exception frame.
      exception = ExceptionCode::kIllegalFunction;
      break;
  }

  if (exception != ExceptionCode::kInvalidFrame) {
    if (exception != ExceptionCode::kOk) {
      resp_buffer_.clear();             // Discard all of the invalid response.
      ResponseAddByte(addr);
      ResponseAddByte(fn_code | 0x80);  // Flag response as exception.
      ResponseAddByte(static_cast<uint8_t>(exception));
    }

    protocol_.WriteFrame({resp_buffer_.begin(), resp_buffer_.end()});
  }

  return true;
}

void Modbus::ResponseAddByte(uint8_t b) { resp_buffer_.push_back(b); }

void Modbus::ResponseAddWord(uint16_t word) {
  // Modbus uses big endian byte order.
  resp_buffer_.push_back(static_cast<uint8_t>(word >> 8));
  resp_buffer_.push_back(static_cast<uint8_t>(word));
}

Modbus::ExceptionCode Modbus::ReadInputRegister(FrameData data) {
  if (data.size() != 4) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t starting_addr = BufferToWordBE(&data[0]);
  uint16_t quantity_regs = BufferToWordBE(&data[2]);

  // Maximum number of registers allowed per spec.
  // This check also prevents buffer overflow of the response buffer.
  if (quantity_regs < 1 || quantity_regs > 0x7D) {
    return ExceptionCode::kIllegalDataValue;
  }

  ResponseAddByte(quantity_regs * 2);  // Byte Count

  // Add all requested registers to the response.
  for (int i = 0; i < quantity_regs; i++) {
    uint16_t reg_content = 0;
    uint16_t addr = starting_addr + i;
    bool ok = data_.ReadRegister(addr, &reg_content);
    if (!ok) {
      return ExceptionCode::kIllegalDataAddress;
    }

    ResponseAddWord(reg_content);
  }

  return ExceptionCode::kOk;
}

Modbus::ExceptionCode Modbus::WriteSingleRegister(FrameData data) {
  if (data.size() != 4) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t wr_addr = BufferToWordBE(&data[0]);
  uint16_t wr_data = BufferToWordBE(&data[2]);

  bool ok = data_.WriteRegister(wr_addr, wr_data);
  if (!ok) {
    return ExceptionCode::kIllegalDataAddress;
  }

  ResponseAddWord(wr_addr);
  ResponseAddWord(wr_data);
  return ExceptionCode::kOk;
}

}  // namespace modbus
