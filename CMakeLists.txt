cmake_minimum_required(VERSION 3.6)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

project(GaMoSy-SSU C ASM)

set(CORTEX_M0_FLAGS "-mcpu=cortex-m0 -mthumb -mfloat-abi=soft")
set(C_FLAGS "-std=c11 -Wall -Wextra -Wstrict-prototypes -Wconversion -fno-common -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORTEX_M0_FLAGS} ${C_FLAGS} -DCORE_M0PLUS")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections,-T${CMAKE_SOURCE_DIR}/src/config/lpc_flash1.ld")

set(CONFIG_INCLUDE ${CMAKE_SOURCE_DIR}/src/config)

add_subdirectory(lib)

add_executable(firmware
  src/startup_LPC82x.s
  src/log.c
  src/main.c
  src/modbus.c
)

target_link_libraries(firmware LPC8xx SEGGER_RTT)
set_target_properties(firmware PROPERTIES OUTPUT_NAME firmware.elf)
