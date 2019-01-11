Soil Sensor Unit
================

Build Instructions
------------------

### Dependencies

* GNU Arm Embedded Toolchain 7-2018-q2-update
  (with 8-2018-q4-major code does not fit into bootloader section anymore)
* Python 3
* CMake 3.12 or higher
* Ninja (optional)

### Firmware

The project consists of a bootloader and the actual firmware image.
A size optimized build must be used for the bootloader or it won’t fit in its
reserved flash memory area.
The firmware image is signed by the mcuboot imgtool scripts which adds a header
and a signature to it. In additional to direct flashing with a programmer the
resulting firmware_image.hex file can be used to update the firmware over
modbus in the field.

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=MinSizeRel -G Ninja ..
    ninja boot # The bootloader only fits with CMAKE_BUILD_TYPE=MinSizeRel
    ninja firmware

### Unit tests

The unit tests use the google test/mock framework and run on the host computer.

    mkdir -p build/test && cd build/test
    cmake -G Ninja ../test
    ninja
    ./ssu_test
