// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "flash_map_backend/flash_map_backend.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "chip.h"

#define NUM_FLASH_AREAS 3
#define SECTOR_SIZE 1024

// Provided by boot.ld linker script in src/config folder.
extern uint8_t _flash_slot0;
extern uint8_t _flash_slot0_end;
extern uint8_t _flash_slot1;
extern uint8_t _flash_slot1_end;
extern uint8_t _flash_scratch;
extern uint8_t _flash_scratch_end;

static struct flash_area areas[NUM_FLASH_AREAS];

void flash_areas_init(void) {
  areas[0].fa_id = 1;
  areas[0].fa_device_id = 0;
  areas[0].fa_off = (uint32_t)&_flash_slot0;
  areas[0].fa_size = (uint32_t)(&_flash_slot0_end - &_flash_slot0);

  areas[1].fa_id = 2;
  areas[1].fa_device_id = 0;
  areas[1].fa_off = (uint32_t)&_flash_slot1;
  areas[1].fa_size = (uint32_t)(&_flash_slot1_end - &_flash_slot1);

  areas[2].fa_id = 3;
  areas[2].fa_device_id = 0;
  areas[2].fa_off = (uint32_t)&_flash_scratch;
  areas[2].fa_size = (uint32_t)(&_flash_scratch_end - &_flash_scratch);
}

int flash_area_open(uint8_t id, const struct flash_area **area) {
  *area = &areas[id - 1];
  return 0;
}

void flash_area_close(const struct flash_area *area) {}

int flash_area_read(const struct flash_area *area, uint32_t off, void *dst,
                    uint32_t len) {
  memcpy(dst, (void *)(area->fa_off + off), len);
  return 0;
}

int flash_area_write(const struct flash_area *area, uint32_t off,
                     const void *src, uint32_t len) {
  uint8_t rc;

  assert(area && src && len > 0 && (len % 4 == 0));

  uint32_t addr = (area->fa_off + off);
  uint32_t sector = addr / SECTOR_SIZE;
  uint32_t sector_addr = sector * SECTOR_SIZE;
  uint32_t sector_offset = addr - sector_addr;
  if (sector_offset + len > SECTOR_SIZE) {
    return -1;
  }

  rc = Chip_IAP_PreSectorForReadWrite(sector, sector);
  assert(rc == IAP_CMD_SUCCESS);

  uint8_t sector_buf[SECTOR_SIZE];

  // Get previous data at this location so it won’t be overwritten.
  if (len < SECTOR_SIZE) {
    memcpy(sector_buf, (void *)sector_addr, SECTOR_SIZE);
  }
  memcpy(&sector_buf[sector_offset], src, len);

  rc = Chip_IAP_CopyRamToFlash(sector_addr, (uint32_t *)&sector_buf[0],
                               SECTOR_SIZE);
  assert(rc == IAP_CMD_SUCCESS);

  return 0;
}

int flash_area_erase(const struct flash_area *area, uint32_t off,
                     uint32_t len) {
  uint8_t rc;
  assert(len == SECTOR_SIZE);

  uint32_t sector_addr = area->fa_off + off;
  assert(sector_addr % SECTOR_SIZE == 0);

  uint32_t sector_start = sector_addr / SECTOR_SIZE;
  uint32_t sector_stop = sector_start + len / SECTOR_SIZE;

  rc = Chip_IAP_PreSectorForReadWrite(sector_start, sector_stop);
  assert(rc == IAP_CMD_SUCCESS);

  rc = Chip_IAP_EraseSector(sector_start, sector_stop);
  assert(rc == IAP_CMD_SUCCESS);

  return 0;
}

int flash_area_read_is_empty(const struct flash_area *area, uint32_t off,
                             void *dst, uint32_t len) {
  int rc = flash_area_read(area, off, dst, len);
  if (rc) {
    return -1;
  }

  uint8_t *u8dst = (uint8_t *)dst;
  for (uint32_t i = 0; i < len; i++) {
    if (u8dst[i] != 0xFF) {
      return 0;
    }
  }

  return 1;
}

int flash_area_get_sectors(int fa_id, uint32_t *count,
                           struct flash_sector *sectors) {
  const struct flash_area *fa = &areas[fa_id - 1];

  uint32_t num_sectors = fa->fa_size / SECTOR_SIZE;
  for (uint32_t i = 0; i < num_sectors; i++) {
    sectors[i].fs_off = SECTOR_SIZE * i;
    sectors[i].fs_size = SECTOR_SIZE;
  }
  *count = num_sectors;

  return 0;
}

uint8_t flash_area_align(const struct flash_area *area) { return 4; }

uint8_t flash_area_erased_val(const struct flash_area *area) { return 0xFF; }

int flash_area_id_from_image_slot(int slot) { return slot + 1; }
int flash_area_id_to_image_slot(int area_id) { return area_id - 1; }
