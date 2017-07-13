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

#ifndef LOG_H_
#define LOG_H_

#define LOG_ERROR(...)   LogFormat(kLogLevelError, __VA_ARGS__)
#define LOG_WARNING(...) LogFormat(kLogLevelWarning, __VA_ARGS__)
#define LOG_INFO(...)    LogFormat(kLogLevelInfo, __VA_ARGS__)
#define LOG_DEBUG(...)   LogFormat(kLogLevelDebug, __VA_ARGS__)
//#define LOG_DEBUG(...)

/// Log levels.
typedef enum {
  kLogLevelError = 0,  ///< Error: User action most likely required.
  kLogLevelWarning,    ///< Warning: Error but no user action required.
  kLogLevelInfo,       ///< Info: Information for users.
  kLogLevelDebug,      ///< Debug: Information for developers.
  kLogLevelMax         ///< Total number of log levels. Only used internally.
} LogLevel;

/// Adds a message to the log.
///
/// @param Log level of the message.
/// @msg   Message to log.
void LogFormat(LogLevel ll, const char *msg, ...);

#endif  // LOG_H_
