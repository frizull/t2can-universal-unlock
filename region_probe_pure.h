#pragma once
#include <stdint.h>
#include <string.h>

// Bench-only Korea comparison: 1 map, 2 country, 3 both. No stored settings.
static inline bool regionProbePatchPure(uint8_t mode, uint32_t id, uint8_t dlc, uint8_t *data) {
  if (!mode || mode > 3 || dlc != 8 || !data) return false;
  uint8_t before[8]; memcpy(before, data, 8);
  if (id == 0x7FF && data[0] == 3 && (mode & 1)) {
    data[1] = (uint8_t)((data[1] & 0xF0) | 7); // GTW_mapRegion: KR.
  } else if (id == 0x7FF && data[0] == 1 && (mode & 2)) {
    // Native FR is bytes 52 46: retain the observed packed-country order.
    data[2] = 'R'; data[3] = 'K';
  } else if (id == 0x238 && (mode & 2)) {
    // Native France=250 matches ISO numeric; Korea=410 is the test hypothesis.
    if (((data[2] | ((uint16_t)data[3] << 8)) & 1023) == 410) return false;
    data[2] = 0x9A; data[3] = (uint8_t)((data[3] & 0xFC) | 1);
    data[6] = (uint8_t)((data[6] & 0x0F) | (((data[6] + 0x10) & 0xF0)));
    uint8_t sum = 0x38 + 2;
    for (uint8_t i = 0; i < 7; ++i) sum = (uint8_t)(sum + data[i]);
    data[7] = sum;
  }
  return memcmp(before, data, 8) != 0;
}

static inline bool regionProbeLivePure(uint8_t mode, uint32_t start, uint32_t now) {
  return mode >= 1 && mode <= 3 && (uint32_t)(now - start) < 180000;
}
