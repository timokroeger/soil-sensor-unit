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

#ifndef EXPECT_H_
#define EXPECT_H_

#include <stdbool.h>

/// Checks expectations about the current state of execution.
///
/// For debug builds a breakpoint is set and execution is halted when the
/// expectation is not met. For release builds thes function is a noop.
///
/// @param expr The expression to check validity for.
#ifndef NDEBUG
static inline void Expect(bool expr)
{
  if (!expr) {
    __asm volatile ("bkpt 0");
  }
}
#else
//static inline void Expect(bool expr) { (void)expr; }
#define Expect(expr)
#endif

#endif  // EXPECT_H_
