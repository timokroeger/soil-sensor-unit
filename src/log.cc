// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "log.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "cmsis.h"
#include "SEGGER_RTT.h"

#include "expect.h"

// Not exported in the "SEGGER_RTT.h" header file but perfectly usable.
extern "C" int SEGGER_RTT_vprintf(unsigned BufferIndex, const char *sFormat,
                                  va_list *pParamList);

static const char log_level_strings[kLogLevelMax][8] = {
    "ERROR  ",
    "WARNING",
    "INFO   ",
    "DEBUG  ",
};

// Uses Segger RTT for logging.
void LogFormat(LogLevel ll, const char *msg, ...) {
  Expect(ll < kLogLevelMax);
  Expect(msg != nullptr);

#ifdef LOG_COLOR
  static const char *log_level_color[kLogLevelMax] = {
      RTT_CTRL_TEXT_RED, RTT_CTRL_TEXT_YELLOW, RTT_CTRL_RESET, RTT_CTRL_RESET};
  SEGGER_RTT_printf(0, "%s", log_level_color[ll]);
#endif

  uint32_t interrupt_number = __get_IPSR();
  if (interrupt_number != 0) {
    // Print interrupt number when called from interrupt.
    SEGGER_RTT_printf(0, "%s [ISR%u] ", log_level_strings[ll],
                      interrupt_number);
  } else {
    // Print nothing when called before scheduler is started.
    SEGGER_RTT_printf(0, "%s ", log_level_strings[ll]);
  }

  va_list ap;
  va_start(ap, msg);
  SEGGER_RTT_vprintf(0, msg, &ap);
  va_end(ap);

  SEGGER_RTT_Write(0, "\r\n", 2);
}
