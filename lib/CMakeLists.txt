add_library(LPC8xx
  lpc_chip_82x/src/acmp_8xx.c
  lpc_chip_82x/src/adc_8xx.c
  lpc_chip_82x/src/chip_8xx.c
  lpc_chip_82x/src/clock_8xx.c
  lpc_chip_82x/src/crc_8xx.c
  lpc_chip_82x/src/dma_8xx.c
  lpc_chip_82x/src/gpio_8xx.c
  lpc_chip_82x/src/i2cm_8xx.c
  lpc_chip_82x/src/i2cs_8xx.c
  lpc_chip_82x/src/i2c_common_8xx.c
  lpc_chip_82x/src/iap.c
  lpc_chip_82x/src/iocon_8xx.c
  lpc_chip_82x/src/irc_8xx.c
  lpc_chip_82x/src/pinint_8xx.c
  lpc_chip_82x/src/pmu_8xx.c
  lpc_chip_82x/src/ring_buffer.c
  lpc_chip_82x/src/sct_8xx.c
  lpc_chip_82x/src/sct_pwm_8xx.c
  lpc_chip_82x/src/spim_8xx.c
  lpc_chip_82x/src/spis_8xx.c
  lpc_chip_82x/src/spi_8xx.c
  lpc_chip_82x/src/stopwatch_8xx.c
  lpc_chip_82x/src/swm_8xx.c
  lpc_chip_82x/src/syscon_8xx.c
  lpc_chip_82x/src/sysinit_8xx.c
  lpc_chip_82x/src/uart_8xx.c
  lpc_chip_82x/src/wkt_8xx.c
  lpc_chip_82x/src/wwdt_8xx.c
)

target_include_directories(LPC8xx SYSTEM
  PUBLIC lpc_chip_82x/inc
)

target_compile_options(LPC8xx PRIVATE -Wno-conversion -Wno-sign-compare -Wno-sign-conversion -Wno-unused-parameter)
