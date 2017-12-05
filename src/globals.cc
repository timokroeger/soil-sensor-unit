// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "globals.h"

ModbusData modbus_data;
ModbusHw modbus_hardware;
Modbus modbus(&modbus_data, &modbus_hardware);
