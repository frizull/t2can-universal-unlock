#include <cassert>
#include <cstring>
#include <cstdio>
#include "../region_probe_pure.h"
int main() {
  const uint8_t stockMap[8] = {3, 0xB1, 8, 0x6D, 0xE0, 0x40, 0x1E, 0};
  const uint8_t stockCountry[8] = {1, 0xA5, 0x52, 0x46, 2, 0x7D, 0xE1, 0};
  const uint8_t stockRoad[8] = {0xF0, 0x21, 0xFA, 0xD8, 0x1F, 0, 0xF6, 0x32};
  for (uint8_t mode = 0; mode <= 4; ++mode) {
    uint8_t raw[8];
    memcpy(raw, stockMap, 8);
    assert(regionProbePatchPure(mode, 0x7FF, 8, raw) == (mode == 1 || mode == 3));
    if (mode == 1 || mode == 3) { assert(raw[1] == 0xB7); raw[1] = 0xB1; }
    assert(!memcmp(raw, stockMap, 8));
    memcpy(raw, stockCountry, 8);
    assert(regionProbePatchPure(mode, 0x7FF, 8, raw) == (mode == 2 || mode == 3));
    if (mode == 2 || mode == 3) { assert(raw[2] == 'R' && raw[3] == 'K'); raw[3] = 0x46; }
    assert(!memcmp(raw, stockCountry, 8));
    memcpy(raw, stockRoad, 8);
    assert(regionProbePatchPure(mode, 0x238, 8, raw) == (mode == 2 || mode == 3));
    if (mode == 2 || mode == 3) {
      assert(raw[2] == 0x9A && raw[3] == 0xD9 && raw[6] == 6);
      assert(raw[7] == (uint8_t)(0x3A + 0xF0 + 0x21 + 0x9A + 0xD9 + 0x1F + 6));
      assert(!regionProbePatchPure(mode, 0x238, 8, raw));
      raw[2] = 0xFA; raw[3] = 0xD8; raw[6] = 0xF6; raw[7] = 0x32;
    }
    assert(!memcmp(raw, stockRoad, 8));
    assert(!regionProbePatchPure(mode, 0x239, 8, raw));
    assert(!regionProbePatchPure(mode, 0x238, 7, raw));
    assert(!regionProbePatchPure(mode, 0x238, 9, raw));
    assert(!regionProbePatchPure(mode, 0x238, 8, nullptr));
  }
  assert(regionProbeLivePure(3, 0xFFFFFFF0, 100));
  assert(regionProbeLivePure(1, 10, 180009));
  assert(!regionProbeLivePure(1, 10, 180010));
  assert(!regionProbeLivePure(0, 10, 11));
  assert(regionProbeSelectPure(0, true, true) == 2); // Restored preference, no timer/host.
  assert(regionProbeSelectPure(3, true, true) == 2); // Persistent country never changes map region.
  assert(regionProbeSelectPure(3, true, false) == 0); // Confirm-Free OFF stops the assist.
  assert(regionProbeSelectPure(1, false, false) == 1); // Temporary experiments retain their selector.
  puts("Region comparison: field preservation, checksum/counter, selectors, malformed RX and expiry PASS");
}
