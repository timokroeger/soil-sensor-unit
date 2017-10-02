cmake_minimum_required(VERSION 3.1)

project(ssu_test)

# Disbale pthreads feature for compatiblity with MinGW. Google test is confused
# if pthreads is available on windows.
if(WIN32)
  set(gtest_disable_pthreads ON CACHE BOOL "Disable pthreads for google test.")
endif()
add_subdirectory(../lib/googletest ./lib/googletest)
include_directories(SYSTEM ../lib/googletest/googlemock/include ../lib/googletest/googletest/include)

# Build test executable.
include_directories(../src)
add_executable(ssu_test ${SRC_BASE} modbus_test.cc ../src/modbus.cc)
target_link_libraries(ssu_test gmock gmock_main)

# Enable as much warnings as possible.
target_compile_definitions(ssu_test PUBLIC UNIT_TEST)
target_compile_options(ssu_test PUBLIC
    -Wall -Wextra -Wconversion -Woverloaded-virtual -Wreorder -Wsign-promo)
target_compile_features(ssu_test PUBLIC cxx_std_11)

option(ASAN_UBSAN "Enable address & undefined behavior sanitizer." OFF)

# Enable runtime checks if requested.
if(ASAN_UBSAN)
  message(STATUS "Enabling address sanitizer and undefined behavior sanitizer.")
  set_property(TARGET ssu_test APPEND_STRING PROPERTY COMPILE_FLAGS " -fsanitize=address -fsanitize=undefined -fsanitize=integer -fno-omit-frame-pointer")
  set_property(TARGET ssu_test APPEND_STRING PROPERTY LINK_FLAGS " -fsanitize=address -fsanitize=undefined -fsanitize=integer")
endif()
