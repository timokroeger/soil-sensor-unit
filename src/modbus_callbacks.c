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

#include "modbus_callbacks.h"

#include "chip.h"

#define OSC_FREQ 12000000u

void ModbusSerialSend(uint8_t *data, int length)
{
  Chip_UART_SendBlocking(LPC_USART0, data, length);
}

void ModbusStartTimer(void)
{
  Chip_MRT_SetInterval(LPC_MRT_CH0, ((OSC_FREQ / 1000000) * 750) | MRT_INTVAL_LOAD);
  Chip_MRT_SetInterval(LPC_MRT_CH1, ((OSC_FREQ / 1000000) * 1750 ) | MRT_INTVAL_LOAD);
}

bool ModbusReadRegister(uint16_t address, uint16_t *data_out)
{
  *data_out = 0u;
  return true;
}
