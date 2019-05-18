// Copyright (c) 2019 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_MODBUS_H_
#define MODBUS_MODBUS_H_

#include "etl/vector.h"

namespace modbus {

using Buffer = etl::vector<uint8_t, 256>;

enum class FunctionCode {
  kReadInputRegister = 4,
  kWriteSingleRegister = 6,
  kWriteMultipleRegisters = 16,
};

enum class ExceptionCode {
  kInvalidFrame = -1,
  kOk = 0,
  kIllegalFunction,
  kIllegalDataAddress,
  kIllegalDataValue,
};

}  // namespace modbus

#endif  // MODBUS_MODBUS_H_
