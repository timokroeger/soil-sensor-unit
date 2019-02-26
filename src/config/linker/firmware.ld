INCLUDE memory_stm32g071x8.ld

REGION_ALIAS("FLASH", FLASH_SLOT0);

/* Actual header is only 32 byte but vector table reloaction requires an
 * alignment of 256.
 * Same value must used in the cmake/mcuboot.cmake script. */
_image_header_size = 256;

INCLUDE flash_map.ld
INCLUDE common.ld
