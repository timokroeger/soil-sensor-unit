#include "modbus/modbus.h"

namespace modbus {

bool Modbus::Execute() {
  if (!protocol_.FrameAvailable()) {
    return false;
  }

  response_.restart();

  etl::const_array_view<uint8_t> fd = protocol_.ReadFrame();
  etl::bit_stream request(const_cast<uint8_t*>(fd.begin()), fd.size());

  // TODO: Allow broadcasts (addr = 0)
  uint8_t addr;
  if (!request.get<uint8_t>(addr) && addr != address_) {
    return false;
  }
  response_.put(addr);

  uint8_t fn_code;
  if (!request.get<uint8_t>(fn_code)) {
    return false;
  }
  response_.put(fn_code);

  ExceptionCode exception = ExceptionCode::kOk;
  switch (static_cast<FunctionCode>(fn_code)) {
    case FunctionCode::kReadInputRegister:
      exception = ReadInputRegister(request);
      break;

    case FunctionCode::kWriteSingleRegister:
      exception = WriteSingleRegister(request);
      break;

    case FunctionCode::kWriteMultipleRegisters:
      exception = WriteMultipleRegisters(request);
      break;

    default:
      // Function code is not supported: Reply with an exception frame.
      exception = ExceptionCode::kIllegalFunction;
      break;
  }

  if (exception == ExceptionCode::kOk) {
    if (!request.at_end()) {
      // Additional bytes at the end also indicate
      return false;
    }
  } else {
    // Forge exception response.
    response_.restart();  // Discard all of the invalid response.
    response_.put(addr);
    response_.put<uint8_t>(fn_code | 0x80);  // Flag response as exception.
    response_.put<uint8_t>(static_cast<uint8_t>(exception));
  }

  protocol_.WriteFrame({response_.begin(), response_.end()});
  return true;
}

Modbus::ExceptionCode Modbus::ReadInputRegister(etl::bit_stream& data) {
  uint16_t starting_addr;
  if (!data.get<uint16_t>(starting_addr)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t quantity_regs;
  if (!data.get<uint16_t>(quantity_regs)) {
    return ExceptionCode::kInvalidFrame;
  }

  // Maximum number of registers allowed per spec.
  // This check also prevents buffer overflow of the response buffer.
  if (quantity_regs < 1 || quantity_regs > 0x7D) {
    return ExceptionCode::kIllegalDataValue;
  }

  response_.put<uint8_t>(quantity_regs * 2);  // Byte Count

  // Add all requested registers to the response.
  for (int i = 0; i < quantity_regs; i++) {
    uint16_t reg_content = 0;
    uint16_t addr = starting_addr + i;
    bool ok = data_.ReadRegister(addr, &reg_content);
    if (!ok) {
      return ExceptionCode::kIllegalDataAddress;
    }

    response_.put(reg_content);
  }

  return ExceptionCode::kOk;
}

Modbus::ExceptionCode Modbus::WriteSingleRegister(etl::bit_stream& data) {
  uint16_t wr_addr;
  if (!data.get<uint16_t>(wr_addr)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t wr_data;
  if (!data.get<uint16_t>(wr_data)) {
    return ExceptionCode::kInvalidFrame;
  }

  bool ok = data_.WriteRegister(wr_addr, wr_data);
  if (!ok) {
    return ExceptionCode::kIllegalDataAddress;
  }

  response_.put(wr_addr);
  response_.put(wr_data);
  return ExceptionCode::kOk;
}

Modbus::ExceptionCode Modbus::WriteMultipleRegisters(etl::bit_stream& data) {
  uint16_t starting_addr;
  if (!data.get<uint16_t>(starting_addr)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t quantity_regs;
  if (!data.get<uint16_t>(quantity_regs)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint8_t byte_count;
  if (!data.get<uint8_t>(byte_count)) {
    return ExceptionCode::kInvalidFrame;
  }

  if (quantity_regs < 1 || quantity_regs > 0x7B ||
      byte_count != (quantity_regs * 2)) {
    return ExceptionCode::kIllegalDataValue;
  }

  for (uint16_t i = 0; i < quantity_regs; i++) {
    uint16_t addr = static_cast<uint16_t>(starting_addr + i);

    uint16_t reg_value;
    if (!data.get<uint16_t>(reg_value)) {
      return ExceptionCode::kInvalidFrame;
    }

    bool ok = data_.WriteRegister(addr, reg_value);
    if (!ok) {
      return ExceptionCode::kIllegalDataAddress;
    }
  }

  response_.put(starting_addr);
  response_.put(quantity_regs);
  return ExceptionCode::kOk;
}

}  // namespace modbus
