MEMORY {
  FLASH_BOOT    : ORIGIN = 0x00000000, LENGTH = 7K
  FLASH_SCRATCH : ORIGIN = 0x00001C00, LENGTH = 1K
  FLASH_SLOT0   : ORIGIN = 0x00002000, LENGTH = 12K
  FLASH_SLOT1   : ORIGIN = 0x00005000, LENGTH = 12K
  RAM           : ORIGIN = 0x10000000, LENGTH = 8K
}

_flash_slot0           = ORIGIN(FLASH_SLOT0);
_flash_slot0_length    = LENGTH(FLASH_SLOT0);
_flash_slot1           = ORIGIN(FLASH_SLOT1);
_flash_slot1_length    = LENGTH(FLASH_SLOT1);
_flash_scratch         = ORIGIN(FLASH_SCRATCH);
_flash_scratch_length  = LENGTH(FLASH_SCRATCH);
