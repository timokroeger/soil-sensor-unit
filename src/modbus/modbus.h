// Copyright (c) 2019 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef MODBUS_MODBUS_H_
#define MODBUS_MODBUS_H_

#include "etl/vector.h"

namespace modbus {

using Buffer = etl::vector<uint8_t, 256>;

}

#endif  // MODBUS_MODBUS_H_
