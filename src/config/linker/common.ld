ENTRY(Reset_Handler)

/* # Sections */
SECTIONS
{
  PROVIDE(_image_header_size = 0);
  PROVIDE(_estack = ORIGIN(RAM) + LENGTH(RAM));

  /* ## Sections in FLASH */
  /* ### Vector table */
  .isr_vector ORIGIN(FLASH) + _image_header_size :
  {
    _isr_vector_start = .;
    KEEP(*(.isr_vector));
  } >FLASH

  /* ### .text */
  .text :
  {
    *(.text .text.*);

    KEEP(*(.init));
    KEEP(*(.fini));
  } > FLASH

  /* ### libc runtime sections for global data construction / destruction */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array));
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)));
    KEEP(*(.init_array));
    PROVIDE_HIDDEN(__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)));
    KEEP(*(.fini_array));
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  /* ### .rodata */
  .rodata : ALIGN(4)
  {
    *(.rodata .rodata.*);

    . = ALIGN(4); /* 4-byte align the end (VMA) of this section */
  } > FLASH

  /* ## Sections in RAM */
  /* ### .data */
  .data : ALIGN(4)
  {
    *(.data .data.*);

    . = ALIGN(4); /* 4-byte align the end (VMA) of this section */
  } > RAM AT > FLASH

  /* LMA of .data */
  _sidata = LOADADDR(.data);

  /* VMA of .data */
  _sdata = ADDR(.data);
  _edata = ADDR(.data) + SIZEOF(.data);

  /* ### .bss */
  .bss : ALIGN(4)
  {
    *(.bss .bss.* COMMON);

    . = ALIGN(4); /* 4-byte align the end (VMA) of this section */
  } > RAM

  _sbss = ADDR(.bss);
  _ebss = ADDR(.bss) + SIZEOF(.bss);

  /* Required by the newlib _sbrk() implemenation */
  end = _ebss;

  /* ## Discarded sections */
  /DISCARD/ :
  {
    /* Unused exception related info that only wastes space */
    *(.ARM.exidx.*);

    libc.a(*)
    libgcc.a(*)
    libm.a(*)
  }
}

ASSERT(_isr_vector_start % 256 == 0, "The vector table must be 256-byte aligned.");
