find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(mcuboot_image TARGET)
  add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND
    ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/lib/mcuboot/scripts/imgtool.py create
      --align 4
      --version 0.0.1
      --header-size 256 --pad-header
      --slot-size 12288
      ${TARGET}.hex ${TARGET}_image.hex
  )
endfunction()
