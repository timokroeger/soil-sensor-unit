cmake_minimum_required(VERSION 3.13)

set(CMAKE_TOOLCHAIN_FILE cmake/arm-cmake-toolchains/arm-gcc-toolchain.cmake)
include(cmake/arm-cmake-toolchains/utils.cmake)
include(cmake/mcuboot.cmake)

project(GaMoSy-SSU VERSION 0.255 LANGUAGES C CXX)

# Global compiler and linker configuration ------------------------------------------------------ #

set(CPU_FLAGS -mthumb -mcpu=cortex-m0)

set(CMAKE_EXECUTABLE_SUFFIX .elf)
add_compile_options(
  ${CPU_FLAGS}

  # Won’t fit in memory if not optimized for size.
  -ffunction-sections -fdata-sections
  -Os -flto

  # Debug information does not increase binary size (LTO strip it anyway)
  -g

  # Minimize C++ runtime overhead.
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>  # Disable static destructors.
)

add_compile_definitions(NDEBUG)

link_directories(src/config/linker)  # Path of linker scripts
add_link_options(
  ${CPU_FLAGS}
  -Wl,--gc-sections
)

# Third-party libaries configuration-------------------------------------------------------------- #

add_subdirectory(lib)

# MCUboot requires some headers to be provided by the user
target_include_directories(mcuboot PUBLIC src/boot/include) # HACKME: Make private
target_sources(mcuboot PRIVATE src/boot/flash_map_backend.cc)
target_link_libraries(mcuboot PRIVATE LPC8xx)

# Bootloader Target ------------------------------------------------------------------------------ #

add_executable(boot
  src/boot/main.c
  src/startup.cc
)

target_link_libraries(boot mcuboot LPC8xx
  -specs=nosys.specs -specs=nano.specs
  -Wl,-Tboot.ld
)

firmware_size(boot)

generate_object(boot boot.elf elf32-littlearm boot.hex ihex)

# Firmware Target -------------------------------------------------------------------------------- #

add_executable(firmware
  src/assert_impl.cc
  src/bootloader.cc
  src/isr.cc
  src/log.cc
  src/main.cc
  src/measure.cc
  src/modbus_data_fw_update.cc
  src/modbus_data.cc
  src/modbus_serial.cc
  src/modbus/slave.cc
  src/setup.cc
  src/startup.cc
)

# Generate header that defines the version number
configure_file(src/version.h.in src/version.h)

target_include_directories(firmware PUBLIC src ${CMAKE_CURRENT_BINARY_DIR}/src)

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
  mcuboot LPC8xx SEGGER_RTT etl sml
  -specs=nosys.specs -specs=nano.specs
  -Wl,-Tfirmware.ld
)

firmware_size(firmware)

# Generate firmware_image.hex for direct flashing with a programmer and firmware_image.bin for
# updates via MODBUS.
generate_object(firmware firmware.elf elf32-littlearm firmware.hex ihex)
mcuboot_image(firmware firmware.hex firmware_image.hex)
generate_object(firmware firmware_image.hex ihex firmware_image.bin binary)

# If flashed accidentally it is rejected by the bootloader because it has no header or signature.
# Remove it right after building to prevent accidental flashing.
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/firmware.hex)
