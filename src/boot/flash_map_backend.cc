// Copyright (c) 2019 Timo Kröger <timokroeger93+code@gmail.com>

#include "flash_map_backend/flash_map_backend.h"

#include <assert.h>
#include <string.h>
#include <array>

#include "stm32g0xx.h"

// Number of flash areas, always 3 with MCBboot: 
constexpr int kNumFlashAreas = 3;

// Smallest erasable flash block size.
constexpr size_t kPageSize = 2048;

// Minimum aligment and size of a flash write operation.
constexpr uint8_t kWriteSize = 8;

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
  assert(area);

  // Unlock FLASH_CR register access with thes key sequence.
  FLASH->KEYR = 0x45670123;
  FLASH->KEYR = 0xCDEF89AB;

  *area = &areas[id - 1];
  return 0;
}

void flash_area_close(const struct flash_area *area) {
  assert(area);

  FLASH->CR = FLASH_CR_LOCK;
}

int flash_area_read(const struct flash_area *area, uint32_t off, void *dst,
                    uint32_t len) {
  assert(area && dst && len > 0);
  memcpy(dst, (void *)(area->fa_off + off), len);
  return 0;
}

int flash_area_write(const struct flash_area *area, uint32_t off,
                     const void *src_addr, uint32_t len) {
  assert(area && src_addr && len > 0);

  uint32_t dst_addr = area->fa_off + off;
  assert(dst_addr % WRITE_SIZE == 0);
  assert(len % WRITE_SIZE == 0);

  uint32_t *dst = static_cast<uint32_t *>(dst);
  uint32_t *src = static_cast<uint32_t *>(src);

  assert(FLASH->SR == 0);  // No running flash operation or previous error

  // Start programming sequence
  FLASH->CR = FLASH_CR_PG;

  // Write two words (= 32bit) per cycle
  for (uint32_t i = 0; i < len / sizeof(uint32_t); i += 2) {
    dst[i] = src[i];
    dst[i + 1] = src[i + 1];

    // Wait for program sequence to finish (~85us)
    while (FLASH->SR & FLASH_SR_BSY1) continue;
  }

  // Stop programming sequence
  FLASH->CR = 0;

  return 0;
}

int flash_area_erase(const struct flash_area *area, uint32_t off,
                     uint32_t len) {
  uint32_t addr = area->fa_off + off;
  assert(addr % kPageSize == 0);
  assert(len % kPageSize == 0);

  uint32_t start_page = addr / kPageSize;
  uint32_t num_pages = len / kPageSize;

  for (uint32_t i = 0; i < num_pages; i++) {
    assert(FLASH->SR == 0);  // No running flash operation or previous error

    // Update page number and select the erase flag and start the operation
    FLASH->CR =
        FLASH_CR_STRT | (start_page + i) << FLASH_CR_PNB_Pos | FLASH_CR_PER;

    // Wait for erase to finish (~22ms)
    while (FLASH->SR & FLASH_SR_BSY1) continue;
  }

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
  assert(fa_id > 0 && fa_id <= NUM_FLASH_AREAS && count && sectors);

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
