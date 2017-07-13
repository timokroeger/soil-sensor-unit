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

#include "log.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "cmsis.h"
#include "SEGGER_RTT.h"

#include "expect.h"

// Not exported in the "SEGGER_RTT.h" header file but perfectly usable.
extern int SEGGER_RTT_vprintf(unsigned BufferIndex, const char * sFormat,
                              va_list * pParamList);

static const char *log_level_color[kLogLevelMax] = {
  RTT_CTRL_TEXT_RED, RTT_CTRL_TEXT_YELLOW, RTT_CTRL_RESET, RTT_CTRL_RESET,
};

static const char log_level_strings[kLogLevelMax][8] = {
  "ERROR  ", "WARNING", "INFO   ", "DEBUG  ",
};

// Uses Segger RTT for logging.
void LogFormat(LogLevel ll, const char *msg, ...)
{
  Expect(ll < kLogLevelMax);
  Expect(msg != NULL);

  uint32_t interrupt_number = __get_IPSR();
  if (interrupt_number != 0) {
    // Print interrupt number when called from interrupt.
    SEGGER_RTT_printf(0, "%s%s [ISR%u] ", log_level_color[ll],
                      log_level_strings[ll], interrupt_number);
  } else {
    // Print nothing when called before scheduler is started.
    SEGGER_RTT_printf(0, "%s%s ", log_level_color[ll], log_level_strings[ll]);
  }

  va_list ap;
  va_start(ap, msg);
  SEGGER_RTT_vprintf(0, msg, &ap);
  va_end(ap);

  SEGGER_RTT_Write(0, "\r\n", 2);
}
