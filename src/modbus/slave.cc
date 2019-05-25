#include "modbus/slave.h"

namespace modbus {

bool Slave::Execute(const Buffer* req_buffer, Buffer* resp_buffer) {
  // const_cast: No modifying methods like bit_stream.put() can be used.
  etl::bit_stream request(const_cast<uint8_t*>(req_buffer->begin()),
                          req_buffer->size());

  // Set vector to the largest possible size to that the resize() call at the
  // end of the method does not overwrite the data inserted by the bit_stream.
  resp_buffer->resize(resp_buffer->capacity());
  etl::bit_stream response(resp_buffer->data(), resp_buffer->size());

  // TODO: Allow broadcasts (addr = 0)
  uint8_t addr;
  if (!request.get<uint8_t>(addr) || addr != address_) {
    return false;
  }
  response.put(addr);

  uint8_t fn_code;
  if (!request.get<uint8_t>(fn_code)) {
    return false;
  }
  response.put(fn_code);

  ExceptionCode exception = ExceptionCode::kOk;
  FunctionCode fnc = static_cast<FunctionCode>(fn_code);
  data_.Start(fnc);
  switch (fnc) {
    case FunctionCode::kReadInputRegister:
      exception = ReadInputRegister(request, response);
      break;

    case FunctionCode::kWriteSingleRegister:
      exception = WriteSingleRegister(request, response);
      break;

    case FunctionCode::kWriteMultipleRegisters:
      exception = WriteMultipleRegisters(request, response);
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
      return false;
    }
  } else {
    if (exception == ExceptionCode::kInvalidFrame) {
      return false;
    }

    // Forge exception response.
    response.restart();  // Discard all of the invalid response.
    response.put(addr);
    response.put<uint8_t>(fn_code | 0x80);  // Flag response as exception.
    response.put<uint8_t>(static_cast<uint8_t>(exception));
  }

  resp_buffer->resize(response.size());
  return true;
}

ExceptionCode Slave::ReadInputRegister(etl::bit_stream& req,
                                       etl::bit_stream& resp) {
  uint16_t starting_addr;
  if (!req.get<uint16_t>(starting_addr)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t quantity_regs;
  if (!req.get<uint16_t>(quantity_regs)) {
    return ExceptionCode::kInvalidFrame;
  }

  // Maximum number of registers allowed per spec.
  // This check also prevents buffer overflow of the response buffer.
  if (quantity_regs < 1 || quantity_regs > 0x7D) {
    return ExceptionCode::kIllegalDataValue;
  }

  resp.put<uint8_t>(quantity_regs * 2);  // Byte Count

  // Add all requested registers to the response.
  for (int i = 0; i < quantity_regs; i++) {
    uint16_t reg_content = 0;
    uint16_t addr = starting_addr + i;
    ExceptionCode exception = data_.ReadRegister(addr, &reg_content);
    if (exception != ExceptionCode::kOk) {
      return ExceptionCode::kIllegalDataAddress;
    }

    resp.put(reg_content);
  }

  return ExceptionCode::kOk;
}

ExceptionCode Slave::WriteSingleRegister(etl::bit_stream& req,
                                         etl::bit_stream& resp) {
  uint16_t wr_addr;
  if (!req.get<uint16_t>(wr_addr)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t wr_data;
  if (!req.get<uint16_t>(wr_data)) {
    return ExceptionCode::kInvalidFrame;
  }

  ExceptionCode exception = data_.WriteRegister(wr_addr, wr_data);
  if (exception != ExceptionCode::kOk) {
    return ExceptionCode::kIllegalDataAddress;
  }

  resp.put(wr_addr);
  resp.put(wr_data);
  return ExceptionCode::kOk;
}

ExceptionCode Slave::WriteMultipleRegisters(etl::bit_stream& req,
                                            etl::bit_stream& resp) {
  uint16_t starting_addr;
  if (!req.get<uint16_t>(starting_addr)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint16_t quantity_regs;
  if (!req.get<uint16_t>(quantity_regs)) {
    return ExceptionCode::kInvalidFrame;
  }

  uint8_t byte_count;
  if (!req.get<uint8_t>(byte_count)) {
    return ExceptionCode::kInvalidFrame;
  }

  if (quantity_regs < 1 || quantity_regs > 0x7B ||
      byte_count != (quantity_regs * 2)) {
    return ExceptionCode::kIllegalDataValue;
  }

  for (uint16_t i = 0; i < quantity_regs; i++) {
    uint16_t addr = static_cast<uint16_t>(starting_addr + i);

    uint16_t reg_value;
    if (!req.get<uint16_t>(reg_value)) {
      return ExceptionCode::kInvalidFrame;
    }

    ExceptionCode exception = data_.WriteRegister(addr, reg_value);
    if (exception != ExceptionCode::kOk) {
      return ExceptionCode::kIllegalDataAddress;
    }
  }

  resp.put(starting_addr);
  resp.put(quantity_regs);
  return ExceptionCode::kOk;
}

}  // namespace modbus
