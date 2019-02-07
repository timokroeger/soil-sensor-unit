cmake_minimum_required(VERSION 3.9)

project(ssu_test)

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(../lib/etl ./lib/etl)
add_subdirectory(../lib/sml ./lib/sml)
add_subdirectory(../lib/googletest ./lib/googletest EXCLUDE_FROM_ALL)

# Build test executable.
include_directories(../src)
add_executable(ssu_test
  ../src/modbus_data_fw_update.cc
  ../src/modbus/modbus.cc
  modbus_data_fw_update_test.cc
  modbus/modbus_test.cc
  modbus/rtu_protocol_test.cc
)
target_link_libraries(ssu_test etl sml gmock_main)

option(ASAN_UBSAN "Enable address & undefined behavior sanitizer." OFF)

# Enable runtime checks if requested.
if(ASAN_UBSAN)
  message(STATUS "Enabling address sanitizer and undefined behavior sanitizer.")
  set_property(TARGET ssu_test APPEND_STRING PROPERTY COMPILE_FLAGS " -fsanitize=address -fsanitize=undefined -fsanitize=integer -fno-omit-frame-pointer")
  set_property(TARGET ssu_test APPEND_STRING PROPERTY LINK_FLAGS " -fsanitize=address -fsanitize=undefined -fsanitize=integer")
endif()
