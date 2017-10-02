// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef EXPECT_H_
#define EXPECT_H_

/// Checks expectations about the current state of execution.
///
/// For debug builds a breakpoint is set and execution is halted when the
/// expectation is not met. For release builds thes function is a noop.
///
/// @param expr The expression to check validity for.
#ifdef UNIT_TEST
// Make the program fail so that we use can death tests.
#include <stdlib.h>
static inline void Expect(bool expr) {
  if (!expr) {
    exit(EXIT_FAILURE);
  }
}
#else
#ifndef NDEBUG
static inline void Expect(bool expr) {
  if (!expr) {
    __asm volatile("bkpt 0");
  }
}
#else
static inline void Expect(bool) {}
#endif
#endif

#endif  // EXPECT_H_
