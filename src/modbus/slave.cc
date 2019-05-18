#include "modbus/slave.h"

namespace modbus {

Buffer* Slave::Execute(const Buffer* req_buffer) {
  // const_cast: No modifying methods like bit_stream.put() can be used.
  etl::bit_stream request(const_cast<uint8_t*>(req_buffer->begin()),
                          req_buffer->size());

  // Set vector to the largest possible size to that the resize() call at the
  // end of the method does not overwrite the data inserted by the bit_stream.
  resp_buffer_.resize(resp_buffer_.capacity());
  response_.restart();

  // TODO: Allow broadcasts (addr = 0)
  uint8_t addr;
  if (!request.get<uint8_t>(addr) || addr != address_) {
    return nullptr;
  }
  response_.put(addr);

  uint8_t fn_code;
  if (!request.get<uint8_t>(fn_code)) {
    return nullptr;
  }
  response_.put(fn_code);

  ExceptionCode exception = ExceptionCode::kOk;
  FunctionCode fnc = static_cast<FunctionCode>(fn_code);
  data_.Start(fnc);
  switch (fnc) {
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

  data_.Complete();

  if (exception == ExceptionCode::kOk) {
    if (!request.at_end()) {
      // Additional bytes at the end make a frame invalid.
      return nullptr;
    }
  } else {
    if (exception == ExceptionCode::kInvalidFrame) {
      return nullptr;
    }

    // Forge exception response.
    response_.restart();  // Discard all of the invalid response.
    response_.put(addr);
    response_.put<uint8_t>(fn_code | 0x80);  // Flag response as exception.
    response_.put<uint8_t>(static_cast<uint8_t>(exception));
  }

  resp_buffer_.resize(response_.size());
  return &resp_buffer_;
}

ExceptionCode Slave::ReadInputRegister(etl::bit_stream& data) {
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
    ExceptionCode exception = data_.ReadRegister(addr, &reg_content);
    if (exception != ExceptionCode::kOk) {
      return ExceptionCode::kIllegalDataAddress;
    }

    response_.put(reg_content);
  }

  return ExceptionCode::kOk;
}

ExceptionCode Slave::WriteSingleRegister(etl::bit_stream& data) {
  uint16_t wr_addr;
  if (!data.get<uint16_t>(wr_addr)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t wr_data;
  if (!data.get<uint16_t>(wr_data)) {
    return ExceptionCode::kInvalidFrame;
  }

  ExceptionCode exception = data_.WriteRegister(wr_addr, wr_data);
  if (exception != ExceptionCode::kOk) {
    return ExceptionCode::kIllegalDataAddress;
  }

  response_.put(wr_addr);
  response_.put(wr_data);
  return ExceptionCode::kOk;
}

ExceptionCode Slave::WriteMultipleRegisters(etl::bit_stream& data) {
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

    ExceptionCode exception = data_.WriteRegister(addr, reg_value);
    if (exception != ExceptionCode::kOk) {
      return ExceptionCode::kIllegalDataAddress;
    }
  }

  response_.put(starting_addr);
  response_.put(quantity_regs);
  return ExceptionCode::kOk;
}

}  // namespace modbus
