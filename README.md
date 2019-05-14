Soil Sensor Unit
================

Build Instructions
------------------

### Dependencies

* GNU Arm Embedded Toolchain
* Python 3
* CMake 3.13 or higher
* Ninja (optional)

Alternatively you can use docker to setup a build environment:

    docker build -t gamosybuild .
    docker run -it -v $(pwd):/source gamosybuild # For Git Bash on Windows use $(pwd -W)

### Firmware

The project consists of a bootloader and the actual firmware image.
The firmware image is signed by the mcuboot imgtool scripts which adds a header
and a signature to it. In additional to direct flashing with a programmer the
resulting firmware_image.hex file can be used to update the firmware over
modbus in the field.

    mkdir build && cd build
    cmake -G Ninja ..
    ninja boot
    ninja firmware

### Unit tests

The unit tests use the google test/mock framework and run on the host computer.

    mkdir -p build/test && cd build/test
    cmake -G Ninja ../test
    ninja
    ./ssu_test
