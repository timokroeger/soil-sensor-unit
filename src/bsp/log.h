// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef BSP_LOG_H_
#define BSP_LOG_H_

#define LOG_ERROR(...)   LogFormat(kLogLevelError, __VA_ARGS__)
#define LOG_WARNING(...) LogFormat(kLogLevelWarning, __VA_ARGS__)
#define LOG_INFO(...)    LogFormat(kLogLevelInfo, __VA_ARGS__)
#define LOG_DEBUG(...)   LogFormat(kLogLevelDebug, __VA_ARGS__)

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

#endif  // BSP_LOG_H_
