INCLUDE memory.ld
REGION_ALIAS("FLASH", FLASH_BOOT);
_flash_slot0 = ORIGIN(FLASH_SLOT0);
_flash_slot1 = ORIGIN(FLASH_SLOT1);
_flash_scratch = ORIGIN(FLASH_SCRATCH);
INCLUDE common.ld
