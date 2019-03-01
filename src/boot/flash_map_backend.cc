 // Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "flash_map_backend/flash_map_backend.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <array>

#include "chip.h"

// Number of flash areas, always 3 with MCBboot: 
constexpr int kNumFlashAreas = 3;

// Smallest erasable flash block size.
constexpr uint32_t kPageSize = 1024;

// Minimum aligment and size of a flash write operation.
constexpr uint8_t kWriteSize = 4;

// Provided by flash_map.ld linker script in src/config/linker folder.
extern uint32_t _flash_slot0[];
extern uint32_t _flash_slot0_length[];
extern uint32_t _flash_slot1[];
extern uint32_t _flash_slot1_length[];
extern uint32_t _flash_scratch[];
extern uint32_t _flash_scratch_length[];

constexpr struct flash_area areas[kNumFlashAreas] = {
    {0, 0, 0, reinterpret_cast<uint32_t>(_flash_slot0),
     reinterpret_cast<uint32_t>(_flash_slot0_length)},
    {1, 0, 0, reinterpret_cast<uint32_t>(_flash_slot1),
     reinterpret_cast<uint32_t>(_flash_slot1_length)},
    {2, 0, 0, reinterpret_cast<uint32_t>(_flash_scratch),
     reinterpret_cast<uint32_t>(_flash_scratch_length)},
};

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
  uint32_t sector = addr / kPageSize;
  uint32_t sector_addr = sector * kPageSize;
  uint32_t sector_offset = addr - sector_addr;
  if (sector_offset + len > kPageSize) {
    return -1;
  }

  rc = Chip_IAP_PreSectorForReadWrite(sector, sector);
  assert(rc == IAP_CMD_SUCCESS);

  uint8_t sector_buf[kPageSize];

  // Get previous data at this location so it won’t be overwritten.
  if (len < kPageSize) {
    memcpy(sector_buf, (void *)sector_addr, kPageSize);
  }
  memcpy(&sector_buf[sector_offset], src, len);

  rc = Chip_IAP_CopyRamToFlash(sector_addr, (uint32_t *)&sector_buf[0],
                               kPageSize);
  assert(rc == IAP_CMD_SUCCESS);

  return 0;
}

int flash_area_erase(const struct flash_area *area, uint32_t off,
                     uint32_t len) {
  uint8_t rc;
  assert(len == kPageSize);

  uint32_t sector_addr = area->fa_off + off;
  assert(sector_addr % kPageSize == 0);

  uint32_t sector_start = sector_addr / kPageSize;
  uint32_t sector_stop = sector_start + len / kPageSize - 1;

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
    return rc;
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

  uint32_t num_sectors = fa->fa_size / kPageSize;
  for (uint32_t i = 0; i < num_sectors; i++) {
    sectors[i].fs_off = kPageSize * i;
    sectors[i].fs_size = kPageSize;
  }
  *count = num_sectors;

  return 0;
}

uint8_t flash_area_align(const struct flash_area *area) { return kWriteSize; }

uint8_t flash_area_erased_val(const struct flash_area *area) { return 0xFF; }

int flash_area_id_from_image_slot(int slot) { return slot + 1; }
int flash_area_id_to_image_slot(int area_id) { return area_id - 1; }
