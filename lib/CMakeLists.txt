add_library(CMSIS INTERFACE)
target_include_directories(CMSIS INTERFACE CMSIS/Include)

add_library(STM32G0xx_CMSIS INTERFACE)
target_include_directories(STM32G0xx_CMSIS INTERFACE STM32G0xx_CMSIS/Include)
target_link_libraries(STM32G0xx_CMSIS INTERFACE CMSIS)

add_library(STM32G0xx_LL OBJECT
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_adc.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_comp.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_crc.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_dac.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_dma.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_exti.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_gpio.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_i2c.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_lptim.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_lpuart.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_pwr.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rcc.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rng.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rtc.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_spi.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_tim.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_ucpd.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_usart.c
  STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_utils.c
)
target_compile_definitions(STM32G0xx_LL PUBLIC USE_FULL_ASSERT USE_FULL_LL_DRIVER)
target_include_directories(STM32G0xx_LL PUBLIC STM32G0xx_HAL_Driver/Inc)
target_link_libraries(STM32G0xx_LL PUBLIC STM32G0xx_CMSIS)

add_library(SEGGER_RTT OBJECT
  SEGGER/SEGGER_RTT.c
  SEGGER/SEGGER_RTT_printf.c
)

target_include_directories(SEGGER_RTT PUBLIC SEGGER)

add_library(mcuboot OBJECT
  mcuboot/boot/bootutil/src/bootutil_misc.c
  mcuboot/boot/bootutil/src/caps.c
  mcuboot/boot/bootutil/src/image_validate.c
  mcuboot/boot/bootutil/src/loader.c
  mcuboot/ext/mbedtls/src/asn1parse.c
  mcuboot/ext/tinycrypt/lib/source/sha256.c
  mcuboot/ext/tinycrypt/lib/source/utils.c
)

target_include_directories(mcuboot
  PRIVATE
  mcuboot/ext/mbedtls/include
  mcuboot/ext/tinycrypt/lib/include
  PUBLIC
  mcuboot/boot/bootutil/include
)

add_subdirectory(etl)
add_subdirectory(sml)
