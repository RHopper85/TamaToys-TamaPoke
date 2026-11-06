#include "../decor_catalog.h"
#include <assert.h>
#include <stdio.h>
int main() {
  DecorState d;
  const uint8_t starters[] = {1, 10, 62, 11, 63, 64, 61};
  for (uint8_t id : starters) assert(d.owns(id) && d.discovered(id));
  for (uint8_t slot = 0; slot < DECOR_SLOTS; ++slot) assert(d.placed(slot) == 0);
  assert(d.validLayout());
  assert(d.place(0, 1));
  assert(d.place(1, 10));
  uint8_t raw[DecorState::WIRE_BYTES];
  d.encode(raw);
  DecorState restored;
  assert(restored.decode(raw, sizeof(raw)));
  assert(restored.placed(0) == 1 && restored.placed(1) == 10);
  restored.resetToStarters();
  for (uint8_t slot = 0; slot < DECOR_SLOTS; ++slot) assert(restored.placed(slot) == 0);
  for (uint8_t id : starters) assert(restored.owns(id));
  puts("PASS: starter ownership, default OFF, saved placements, reset OFF");
}
