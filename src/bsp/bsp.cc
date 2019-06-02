// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "bsp/bsp.h"

#include <algorithm>

#include "chip.h"

// Required by the vendor chip library.
extern "C" {

const uint32_t OscRateIn = 0;  // External oscillator not used.
const uint32_t ExtRateIn = 0;  // External clock input not used.

}

Bootloader bootloader;
ModbusSerial modbus_serial(LPC_USART0, LPC_MRT_CH0);

constexpr int kMeasurementStartDelayMs = 5;

namespace {

void SetPower(uint32_t mode) {
  uint32_t clk_mhz = SystemCoreClock / 1'000'000;
  uint32_t param[] = {clk_mhz, mode, clk_mhz};
  uint32_t result;
  LPC_ROM_API->pPWRD->set_power(param, &result);
  assert(result == PWR_CMD_SUCCESS);
}

void UseIrc() {
  Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_IRC);
  Chip_Clock_SetSysClockDiv(1);
  Chip_FMC_SetFLASHAccess(FLASHTIM_20MHZ_CPU);
  Chip_SYSCTL_PowerDown(SYSCTL_SLPWAKE_SYSPLL_PD);

  // Update CMSIS clock frequency variable which is used in iap.c
  SystemCoreClock = 12'000'000;

  SetPower(PWR_LOW_CURRENT);
}

void UsePll() {
  SetPower(PWR_DEFAULT);

  Chip_SYSCTL_PowerUp(SYSCTL_SLPWAKE_SYSPLL_PD);
  while (!Chip_Clock_IsSystemPLLLocked())
    ;

  // Main clock frequency: 60MHz / 2 = 30Mhz
  Chip_FMC_SetFLASHAccess(FLASHTIM_30MHZ_CPU);
  Chip_Clock_SetSysClockDiv(2);
  Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLOUT);

  // Update CMSIS clock frequency variable which is used in iap.c
  SystemCoreClock = 30000000;
}

// Configures system clock to 30MHz with the PLL fed by the internal oscillator.
void SetupClock() {
  UseIrc();

  // Cannot use vendor provided routine in ROM memory to setup the system clock.
  // It does not support setting 30MHz as output frequency.

  // PLL input FCLKIN (10-25MHz): IRC = 12MHz
  // PLL output FCLKOUT (<100MHz): 5 * 12MHz = 60MHz
  // PLL intrenal FCCO (156-320MHz): 2 * 2 * 60MHz = 240Mhz
  Chip_Clock_SetSystemPLLSource(SYSCTL_PLLCLKSRC_IRC);
  Chip_Clock_SetupSystemPLL(4, 1);
}

// Enables LED output and sets ADC pins to analog mode.
void SetupGpio() {
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);

  // Disable pull-up on DE for the RS485 transceiver.
  // An external pull-down resistor is used instead.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO0, PIN_MODE_INACTIVE);

  // Disable pull-ups for PWM output.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO4, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO12, PIN_MODE_INACTIVE);

  // Disable pull-ups on outputs to save power.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO1, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO9, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO10, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO11, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO15, PIN_MODE_INACTIVE);

  // Disable pull-ups for the analog inputs.
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO13, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO17, PIN_MODE_INACTIVE);
  Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO23, PIN_MODE_INACTIVE);

  Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_IOCON);

  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_GPIO);

  // Set LED pin and all unused pins as output.
  LPC_GPIO_PORT->DIRSET[0] =
      (1 << 15) | (1 << 11) | (1 << 10) | (1 << 9) | (1 << 1);

  // Turn on LED.
  // LPC_GPIO_PORT->SET[0] = (1 << 9);

  Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_GPIO);
}

// Assigns peripherals to pins.
void SetupSwichMatrix() {
  // Enable switch matrix.
  Chip_SWM_Init();

  // UART0
  Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 8);
  Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, 14);
  Chip_SWM_MovablePinAssign(SWM_U0_RTS_O, 0);

  // PWM output
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT0_O, 4);   // FREQ_LO
  Chip_SWM_MovablePinAssign(SWM_SCT_OUT1_O, 12);  // FREQ_HI

  // ADC input
  Chip_SWM_EnableFixedPin(SWM_FIXED_ADC3);
  Chip_SWM_EnableFixedPin(SWM_FIXED_ADC9);
  Chip_SWM_EnableFixedPin(SWM_FIXED_ADC10);

  // Switch matrix clock is not needed anymore after configuration.
  Chip_SWM_Deinit();
}

// Calibrates the ADC and sets up a measurement sequence.
void SetupAdc() {
  Chip_ADC_Init(LPC_ADC, 0);

  // Wait for ADC to be calibrated.
  Chip_ADC_StartCalibration(LPC_ADC);
  while (!Chip_ADC_IsCalibrationDone(LPC_ADC))
    ;

  // Discard calibration samples.
  while (Chip_ADC_GetSequencerDataReg(LPC_ADC, ADC_SEQA_IDX) &
         ADC_SEQ_GDAT_DATAVALID)
    ;

  // Revert changes that were done by the calibration routine.
  // Set ADC clock: A value of 0 divides the system clock by 1.
  Chip_ADC_SetDivider(LPC_ADC, 0);
  LPC_ADC->CTRL |= ADC_CR_LPWRMODEBIT;

  Chip_ADC_SetupSequencer(LPC_ADC, ADC_SEQA_IDX,
                          ADC_SEQ_CTRL_CHANSEL(3) | ADC_SEQ_CTRL_CHANSEL(9) |
                              ADC_SEQ_CTRL_CHANSEL(10) | ADC_SEQ_CTRL_MODE_EOS |
                              ADC_SEQ_CTRL_HWTRIG_POLPOS);
  Chip_ADC_EnableSequencer(LPC_ADC, ADC_SEQA_IDX);

  Chip_ADC_EnableInt(LPC_ADC, ADC_INTEN_SEQA_ENABLE);
}

// Sets up a PWM output with 50% duty cycle used as excitation signal for the
// capacitive measurement.
void SetupPwm() {
  Chip_SCT_Init(LPC_SCT);

  // Set output frequency with the reload match 0 value. This value is loaded
  // to the match register with each limit event.
  // The timer must expire two times during one PWM cycle (PWM_FREQ * 2) to
  // create complementary outputs with two timer states.
  // Use the fastet possible pwm frequency by setting the reload value to 0:
  // 30MHz / 2 = 15MHz
  LPC_SCT->MATCHREL[0].L = 0;

  // Link events to states.
  LPC_SCT->EV[0].STATE = (1 << 0);  // Event 0 only happens in state 0
  LPC_SCT->EV[1].STATE = (1 << 1);  // Event 1 only happens in state 1

  // Add alternating states which are switched by each match event
  LPC_SCT->EV[0].CTRL =
      (0 << 0) |   // Use match register 0 for comparison
      (1 << 12) |  // COMBMODE[13:12] = Change state on match
      (1 << 14) |  // STATELD[14] = STATEV is loaded into state
      (1 << 15);   // STATEV[19:15] = New state is 1
  LPC_SCT->EV[1].CTRL =
      (0 << 0) |   // Use match register 0 for comparison
      (1 << 12) |  // COMBMODE[13:12] = Change state on match
      (1 << 14) |  // STATELD[14] = STATEV is loaded into state
      (0 << 15);   // STATEV[19:15] = New state is 0

  // FREQ_LO: LOW during first half of period, HIGH for the second half.
  LPC_SCT->OUT[0].SET = (1 << 0);  // Event 0 sets
  LPC_SCT->OUT[0].CLR = (1 << 1);  // Event 1 clears

  // FREQ_HI: HIGH during first half of period, LOW for the second half.
  LPC_SCT->OUT[1].SET = (1 << 1);  // Event 1 sets
  LPC_SCT->OUT[1].CLR = (1 << 0);  // Event 0 clears

  // Restart counter on event 0 and 1 (match occurred)
  LPC_SCT->LIMIT_L = (1 << 0) | (1 << 1);
}

// Use the multirate timer for various timing related like delays.
void SetupTimers() { Chip_MRT_Init(); }

// Configures and enables interrupts.
void SetupNVIC() {
  NVIC_SetPriority(UART0_IRQn, 1);
  NVIC_EnableIRQ(UART0_IRQn);

  NVIC_SetPriority(MRT_IRQn, 1);
  NVIC_EnableIRQ(MRT_IRQn);
}

}  // namespace

void BspSetup() {
  SetupClock();
  SetupAdc();
  SetupPwm();
  SetupTimers();
  SetupNVIC();
}

void BspSetupPins() {
  SetupGpio();
  SetupSwichMatrix();
}

void BspSleep() { __WFI(); }

void BspReset() { NVIC_SystemReset(); }

uint16_t BspMeasureRaw() {
  // Switch to faster clock.
  UsePll();

  // Start the PWM timer.
  LPC_SCT->CTRL_L &= (uint16_t)~SCT_CTRL_HALT_L;

  // Wait until capacitor is charged.
  Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_WKT);
  Chip_WKT_ClearIntStatus(LPC_WKT);
  Chip_WKT_Start(LPC_WKT, WKT_CLKSRC_DIVIRC,
                 kMeasurementStartDelayMs * 750'000 / 1'000);
  while (!Chip_WKT_GetIntStatus(LPC_WKT))
    ;
  Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_WKT);

  // Start and wait for ADC conversion to finish.
  Chip_ADC_ClearFlags(LPC_ADC, ADC_FLAGS_SEQA_INT_MASK);
  Chip_ADC_StartSequencer(LPC_ADC, ADC_SEQA_IDX);

  // Wait for conversion to be finished.
  while ((Chip_ADC_GetFlags(LPC_ADC) & ADC_FLAGS_SEQA_INT_MASK) == 0)
    ;

  uint16_t low = ADC_DR_RESULT(Chip_ADC_GetDataReg(LPC_ADC, 3));
  uint16_t high = ADC_DR_RESULT(Chip_ADC_GetDataReg(LPC_ADC, 9));
  uint16_t diode = ADC_DR_RESULT(Chip_ADC_GetDataReg(LPC_ADC, 10));

  // Stop the PWM timer.
  LPC_SCT->CTRL_L |= (uint16_t)SCT_CTRL_HALT_L;

  // Go back no normal clock rate to save power.
  UseIrc();

  return high - low + diode;
}

BspInterruptFree::BspInterruptFree() { __disable_irq(); }
BspInterruptFree::~BspInterruptFree() { __enable_irq(); }

// Implementaion for newlib assert()
extern "C" void __assert_func(const char *, int, const char *, const char *) {
  BspInterruptFree _;
  // TODO: Disable LED.
  for (;;)
    ;
}

// Interrupt Service Routines
void MRT_Handler() { modbus_serial.TimerIsr(); }
void UART0_Handler() { modbus_serial.UartIsr(); }
