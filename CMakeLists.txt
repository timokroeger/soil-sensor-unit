cmake_minimum_required(VERSION 3.9)

set(CMAKE_TOOLCHAIN_FILE cmake/arm-cmake-toolchains/arm-gcc-toolchain.cmake)

project(GaMoSy-SSU C CXX)

# Set target architecture settings for everything.
set(CPU_FLAGS -mthumb -mcpu=cortex-m0plus)
set(CMAKE_EXECUTABLE_SUFFIX .elf)

add_compile_options(
  ${CPU_FLAGS}
  -ffunction-sections -fdata-sections

  # Minimize C++ runtime overhead.
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit> # Disable static destructors.
)

add_link_options(
  ${CPU_FLAGS}
  -Wl,--gc-sections
)

# Path of linker scripts
link_directories(src/config/linker)

set(CONFIG_INCLUDE ${CMAKE_SOURCE_DIR}/src/config)
add_subdirectory(lib)

# Bootloader
add_executable(boot
  src/startup.cc
  src/boot/main.c
)

target_link_libraries(boot
  LPC8xx
  -specs=nosys.specs -specs=nano.specs
  -Wl,-Tboot.ld
)

# Actual firmware
add_executable(firmware
  src/modbus/modbus.cc
  src/assert_impl.cc
  src/globals.cc
  src/isr.cc
  src/log.cc
  src/led.cc
  src/main.cc
  src/measure.cc
  src/modbus_data.cc
  src/modbus_serial.cc
  src/setup.cc
  src/startup.cc
)

target_include_directories(firmware PUBLIC src)

target_compile_options(firmware PUBLIC
  -Wall -Wextra -Wno-unused -Wsign-promo
# Reenable those warnings when the -Isystem include bug is fixed in
# arm-none-eabi-gcc (likely) or all warnings are fixed in etl (unlikely)
#  -Wold-style-cast -Wsign-conversion
  -Woverloaded-virtual -Wreorder

  # Template heave code require a little optimization even in debug builds.
  $<$<CONFIG:DEBUG>:-Og>
)
target_compile_features(firmware PUBLIC cxx_std_14)

target_link_libraries(firmware
  LPC8xx SEGGER_RTT etl sml
  -specs=nosys.specs -specs=nano.specs
  -Wl,-Tfirmware.ld
)
#set_property(TARGET firmware PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

# Postprocessing
include(cmake/arm-cmake-toolchains/utils.cmake)

generate_object(boot .hex ihex) 
generate_object(firmware .hex ihex) 

firmware_size(firmware)
