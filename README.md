Soil Sensor Unit
================

Build Instructions
------------------

### CMake + arm-none-eabi Toolchain

On windows a msys2 installation is recommended.
Example build sequence:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -G Ninja <path_to_project>
    ninja

### Unit tests

The unit tests use the google test/mock framework. They run on the host computer
and require gcc compatible C++ compiler. CMake must be used to build the tests.

    cd build_tests
    cmake -G Ninja <path_to_project>/test
    ninja
    ./ssu_test.exe

TODO
----

- Add LICENSE file
