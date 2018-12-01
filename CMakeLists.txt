cmake_minimum_required(VERSION 3.9)

set(CMAKE_TOOLCHAIN_FILE cmake/arm-cmake-toolchains/arm-gcc-toolchain.cmake)

project(GaMoSy-SSU C CXX)

# Set target architecture settings for everything.
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/src/config/lpc_flash1.ld)
set(CORTEX_M0_FLAGS "-mcpu=cortex-m0 -mthumb -fno-common -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORTEX_M0_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORTEX_M0_FLAGS}")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(CONFIG_INCLUDE ${CMAKE_SOURCE_DIR}/src/config)
add_subdirectory(lib)

add_executable(firmware
  src/modbus/modbus.cc
  src/assert_impl.cc
  src/config_storage.cc
  src/globals.cc
  src/isr.cc
  src/log.cc
  src/led.cc
  src/main.cc
  src/measure.cc
  src/modbus.cc
  src/modbus_data.cc
  src/modbus_hw.cc
  src/setup.cc
  src/startup.cc
)

target_include_directories(firmware PUBLIC src)

target_compile_options(firmware PUBLIC
    -fno-exceptions -fno-non-call-exceptions -fno-rtti
    -Wall -Wextra -Wno-unused -Wsign-promo
# Reenable those warnings when the -Isystem include bug is fixed in
# arm-none-eabi-gcc (likely) or all warnings are fixed in etl (unlikely)
#    -Wold-style-cast -Wsign-conversion
    -Woverloaded-virtual -Wreorder)
target_compile_features(firmware PUBLIC cxx_std_14)

target_link_libraries(firmware LPC8xx SEGGER_RTT etl
    -specs=nosys.specs -specs=nano.specs -Wl,--gc-sections,-T${LINKER_SCRIPT})
set_property(TARGET firmware PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

# Postprocessing
include(cmake/arm-cmake-toolchains/utils.cmake)
firmware_size(firmware)
generate_object(firmware .bin binary)
generate_object(firmware .hex ihex) 
