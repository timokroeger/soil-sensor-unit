cmake_minimum_required(VERSION 3.6)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_ASM_COMPILER arm-none-eabi-as)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

project(GaMoSy-SSU ASM C CXX)

set(CORTEX_M0_FLAGS "-mcpu=cortex-m0 -mthumb -mfloat-abi=soft")

# Configure flags and variables for the libraries.
set(CMAKE_C_FLAGS ${CORTEX_M0_FLAGS})
set(CONFIG_INCLUDE ${CMAKE_SOURCE_DIR}/src/config)
add_subdirectory(lib)

add_executable(firmware
  src/startup_LPC82x.s
  src/log.cc
  src/main.cc
  src/measure.cc
  src/modbus.cc
  src/modbus_callbacks.cc
  src/modbus_hw.cc
)

set_property(TARGET firmware PROPERTY COMPILE_FLAGS "${CORTEX_M0_FLAGS}\
    -fno-common -ffunction-sections -fdata-sections \
    -fno-exceptions -fno-non-call-exceptions -fno-rtti\
    -Wall -Wextra -Wconversion -Woverloaded-virtual -Wreorder -Wsign-promo")
set_property(TARGET firmware PROPERTY LINK_FLAGS "${CORTEX_M0_FLAGS}\
    -specs=nosys.specs -specs=nano.specs\
    -Wl,--gc-sections,-T${CMAKE_SOURCE_DIR}/src/config/lpc_flash1.ld")
set_property(TARGET firmware PROPERTY CXX_STANDARD 11)
set_property(TARGET firmware PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(firmware LPC8xx SEGGER_RTT)
set_target_properties(firmware PROPERTIES OUTPUT_NAME firmware.elf)
