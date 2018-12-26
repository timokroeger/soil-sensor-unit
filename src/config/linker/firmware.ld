INCLUDE memory.ld
REGION_ALIAS("FLASH", FLASH_SLOT0);

/* Actual header is only 32 byte but vector table reloaction requires an
 * alignment of 128.
 * Same value must used in the cmake/mcuboot.cmake script. */
_image_header_size = 256;

INCLUDE common.ld
