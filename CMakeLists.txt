cmake_minimum_required(VERSION 3.1)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

project(GaMoSy-SSU ASM C CXX)

set(CONFIG_INCLUDE ${CMAKE_SOURCE_DIR}/src/config)
add_subdirectory(lib)

add_executable(firmware.elf
  src/startup_LPC82x.s
  src/log.cc
  src/main.cc
  src/measure.cc
  src/modbus.cc
  src/modbus_data.cc
  src/modbus_hw.cc
)

target_compile_options(firmware.elf PUBLIC
    -fno-common -ffunction-sections -fdata-sections
    -fno-exceptions -fno-non-call-exceptions -fno-rtti
    -Wall -Wextra -Wconversion -Woverloaded-virtual -Wreorder -Wsign-promo)
target_compile_features(firmware.elf PUBLIC cxx_std_11)

target_link_libraries(firmware.elf LPC8xx SEGGER_RTT
    -specs=nosys.specs -specs=nano.specs
    -Wl,--gc-sections,-T${CMAKE_SOURCE_DIR}/src/config/lpc_flash1.ld)

add_custom_command(TARGET firmware.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} ARGS -O binary firmware.elf firmware.bin)
add_custom_command(TARGET firmware.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex firmware.elf firmware.hex)
