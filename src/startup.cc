// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

extern int main(void);

extern "C" {

extern void _estack(void);         // Provided by linker script.
extern void __libc_init_array();   // Provided by libc.

void Reset_Handler(void);   // Required for ENTRY() in linker script.
void Unused_Handler(void);  // Required for alias attribute.

// Code locations provided by the linker script.
extern unsigned int _sidata;
extern unsigned int _sdata;
extern unsigned int _edata;
extern unsigned int _sbss;
extern unsigned int _ebss;

}

// Create a forward declaration for each interrupt handler
void NMI_Handler()       __attribute__((weak, alias("Unused_Handler")));
void HardFault_Handler() __attribute__((weak, alias("Unused_Handler")));
void SVC_Handler()       __attribute__((weak, alias("Unused_Handler")));
void PendSV_Handler()    __attribute__((weak, alias("Unused_Handler")));
void SysTick_Handler()   __attribute__((weak, alias("Unused_Handler")));
void SPI0_Handler()      __attribute__((weak, alias("Unused_Handler")));
void SPI1_Handler()      __attribute__((weak, alias("Unused_Handler")));
void UART0_Handler()     __attribute__((weak, alias("Unused_Handler")));
void UART1_Handler()     __attribute__((weak, alias("Unused_Handler")));
void UART2_Handler()     __attribute__((weak, alias("Unused_Handler")));
void I2C1_Handler()      __attribute__((weak, alias("Unused_Handler")));
void I2C0_Handler()      __attribute__((weak, alias("Unused_Handler")));
void SCT_Handler()       __attribute__((weak, alias("Unused_Handler")));
void MRT_Handler()       __attribute__((weak, alias("Unused_Handler")));
void CMP_Handler()       __attribute__((weak, alias("Unused_Handler")));
void WDT_Handler()       __attribute__((weak, alias("Unused_Handler")));
void BOD_Handler()       __attribute__((weak, alias("Unused_Handler")));
void FLASH_Handler()     __attribute__((weak, alias("Unused_Handler")));
void WKT_Handler()       __attribute__((weak, alias("Unused_Handler")));
void ADC_SEQA_Handler()  __attribute__((weak, alias("Unused_Handler")));
void ADC_SEQB_Handler()  __attribute__((weak, alias("Unused_Handler")));
void ADC_THCMP_Handler() __attribute__((weak, alias("Unused_Handler")));
void ADC_OVR_Handler()   __attribute__((weak, alias("Unused_Handler")));
void DMA_Handler()       __attribute__((weak, alias("Unused_Handler")));
void I2C2_Handler()      __attribute__((weak, alias("Unused_Handler")));
void I2C3_Handler()      __attribute__((weak, alias("Unused_Handler")));
void PIN_INT0_Handler()  __attribute__((weak, alias("Unused_Handler")));
void PIN_INT1_Handler()  __attribute__((weak, alias("Unused_Handler")));
void PIN_INT2_Handler()  __attribute__((weak, alias("Unused_Handler")));
void PIN_INT3_Handler()  __attribute__((weak, alias("Unused_Handler")));
void PIN_INT4_Handler()  __attribute__((weak, alias("Unused_Handler")));
void PIN_INT5_Handler()  __attribute__((weak, alias("Unused_Handler")));
void PIN_INT6_Handler()  __attribute__((weak, alias("Unused_Handler")));
void PIN_INT7_Handler()  __attribute__((weak, alias("Unused_Handler")));


// Put all interrupt handlers in the vector table.
__attribute__((used, section(".isr_vector"))) void (*vectors[])(void) = {
    _estack,          Reset_Handler,    NMI_Handler,       HardFault_Handler,
    Unused_Handler,   Unused_Handler,   Unused_Handler,    Unused_Handler,
    Unused_Handler,   Unused_Handler,   Unused_Handler,    SVC_Handler,
    Unused_Handler,   Unused_Handler,   PendSV_Handler,    SysTick_Handler,
    SPI0_Handler,     SPI1_Handler,     Unused_Handler,    UART0_Handler,
    UART1_Handler,    UART2_Handler,    Unused_Handler,    I2C1_Handler,
    I2C0_Handler,     SCT_Handler,      MRT_Handler,       CMP_Handler,
    WDT_Handler,      BOD_Handler,      FLASH_Handler,     WKT_Handler,
    ADC_SEQA_Handler, ADC_SEQB_Handler, ADC_THCMP_Handler, ADC_OVR_Handler,
    DMA_Handler,      I2C2_Handler,     I2C3_Handler,      Unused_Handler,
    PIN_INT0_Handler, PIN_INT1_Handler, PIN_INT2_Handler,  PIN_INT3_Handler,
    PIN_INT4_Handler, PIN_INT5_Handler, PIN_INT6_Handler,  PIN_INT7_Handler,
};

void Reset_Handler(void) {
  unsigned int *src, *dst;

  src = &_sidata;
  dst = &_sdata;
  while (dst < &_edata)
    *dst++ = *src++;

  dst = &_sbss;
  while (dst < &_ebss)
    *dst++ = 0;

  __libc_init_array();

  main();
  while (1);
}

void Unused_Handler(void) { while(1); }
