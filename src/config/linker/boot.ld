INCLUDE memory_stm32g071x8.ld

REGION_ALIAS("FLASH", FLASH_BOOT);

INCLUDE flash_map.ld
INCLUDE common.ld
