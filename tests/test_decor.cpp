#include "../decor_catalog.h"
#include <assert.h>
#include <stdio.h>
int main() {
  DecorState d;
  const uint8_t starters[] = {1, 10, 62, 11, 63, 64, 61};
  for (uint8_t id : starters) assert(d.owns(id) && d.discovered(id));
  for (uint8_t slot = 0; slot < DECOR_SLOTS; ++slot) assert(d.placed(slot) == 0);
  assert(d.placedCount(DC_REST) == 0);
  assert(!d.owns(51));
  assert(!d.place(5, 51));  // cannot place unowned rare item
  assert(d.unlock(51));
  assert(d.place(5, 51));
  assert(!d.place(6, 51));  // cannot place duplicate
  assert(d.unlock(52));
  assert(!d.place(6, 52));  // only one special object
  assert(d.place(5, 0));
  assert(d.place(6, 52));
  assert(d.place(0, 1));   // user explicitly switches bed ON
  uint8_t bytes[DecorState::WIRE_BYTES]; d.encode(bytes);
  DecorState restored;
  assert(restored.decode(bytes, sizeof(bytes)));
  assert(restored.owns(51) && restored.placed(6) == 52);
  assert(restored.placed(0) == 1);
  bytes[2 * DECOR_FLAGS_BYTES + 6] = 255;
  assert(!restored.decode(bytes, sizeof(bytes)));
  assert(restored.placed(6) == 52); // invalid restore leaves state unchanged
  DecorState duplicate;
  assert(duplicate.place(0, 1));
  uint8_t dupBytes[DecorState::WIRE_BYTES];
  duplicate.encode(dupBytes);
  dupBytes[2 * DECOR_FLAGS_BYTES + 5] = 1; // duplicate the placed bed
  assert(!duplicate.decode(dupBytes, sizeof(dupBytes)));
  restored.resetToStarters();
  for (uint8_t slot = 0; slot < DECOR_SLOTS; ++slot) assert(restored.placed(slot) == 0);
  puts("PASS: starter owned but OFF, rarity, limits, duplicates, persistence, corruption");
}
