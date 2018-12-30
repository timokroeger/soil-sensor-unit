cmake_minimum_required(VERSION 3.12)

set(CMAKE_TOOLCHAIN_FILE cmake/arm-cmake-toolchains/arm-gcc-toolchain.cmake)

project(GaMoSy-SSU C CXX)

# Set target architecture settings for everything.
set(CPU_FLAGS -mthumb -mcpu=cortex-m0plus)
set(LTO_FLAGS $<$<OR:$<CONFIG:MinSizeRel>,$<CONFIG:Release>>:-flto>)
set(CMAKE_EXECUTABLE_SUFFIX .elf)

add_compile_options(
  ${CPU_FLAGS}
  ${LTO_FLAGS}
  -ffunction-sections -fdata-sections

  # Minimize C++ runtime overhead.
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit> # Disable static destructors.
)

add_link_options(
  ${CPU_FLAGS}
  ${LTO_FLAGS}
  -Wl,--gc-sections
)

# Path of linker scripts
link_directories(src/config/linker)

set(CONFIG_INCLUDE ${CMAKE_SOURCE_DIR}/src/config)
add_subdirectory(lib)

# Boot library
add_library(bootutil OBJECT
  lib/mcuboot/boot/bootutil/src/bootutil_misc.c
  lib/mcuboot/boot/bootutil/src/caps.c
  lib/mcuboot/boot/bootutil/src/image_validate.c
  lib/mcuboot/boot/bootutil/src/loader.c
  lib/mcuboot/ext/mbedtls/src/asn1parse.c
  lib/mcuboot/ext/tinycrypt/lib/source/sha256.c
  lib/mcuboot/ext/tinycrypt/lib/source/utils.c
  src/boot/flash_map_backend.c
)

target_include_directories(bootutil
  PRIVATE
  ${CONFIG_INCLUDE}
  lib/mcuboot/ext/mbedtls/include
  lib/mcuboot/ext/tinycrypt/lib/include
  PUBLIC
  lib/mcuboot/boot/bootutil/include
  src/boot/include
)

target_link_libraries(bootutil LPC8xx)

# Bootloader
add_executable(boot
  src/boot/main.c
  src/startup.cc
)

target_link_libraries(boot
  bootutil LPC8xx
  -specs=nosys.specs -specs=nano.specs
  -Wl,-Tboot.ld
)

# Actual firmware
add_executable(firmware
  src/boot/bootloader.cc
  src/modbus/modbus.cc
  src/assert_impl.cc
  src/globals.cc
  src/isr.cc
  src/log.cc
  src/main.cc
  src/measure.cc
  src/modbus_data.cc
  src/modbus_serial.cc
  src/setup.cc
  src/startup.cc
)

target_include_directories(firmware PUBLIC src)

target_compile_features(firmware PUBLIC cxx_std_14)
target_compile_options(firmware PUBLIC
  -Wall -Wextra -Wno-unused -Wsign-promo
# Reenable those warnings when the -Isystem include bug is fixed in
# arm-none-eabi-gcc (likely) or all warnings are fixed in etl (unlikely)
#  -Wold-style-cast -Wsign-conversion
  -Woverloaded-virtual -Wreorder

  # Template heave code require a little optimization even in debug builds.
  $<$<CONFIG:DEBUG>:-Og>
)

target_link_libraries(firmware
  bootutil LPC8xx SEGGER_RTT etl sml
  -specs=nosys.specs -specs=nano.specs
  -Wl,-Tfirmware.ld
)

# Postprocessing
include(cmake/arm-cmake-toolchains/utils.cmake)
include(cmake/mcuboot.cmake)

generate_object(boot .hex ihex)
generate_object(firmware .hex ihex)
mcuboot_image(firmware)
firmware_size(firmware)
