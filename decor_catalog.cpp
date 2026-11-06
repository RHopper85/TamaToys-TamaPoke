#include "decor_catalog.h"
#include <string.h>

static bool bitGet(const uint8_t *a, uint8_t id) { return (a[id >> 3] & (1u << (id & 7))) != 0; }
static void bitSet(uint8_t *a, uint8_t id) { a[id >> 3] |= (uint8_t)(1u << (id & 7)); }

DecorState::DecorState() { resetToStarters(); }
void DecorState::resetToStarters() {
  memset(owned_, 0, sizeof(owned_));
  memset(discovered_, 0, sizeof(discovered_));
  memset(slots_, 0, sizeof(slots_));
  // Simple Bed, Food Bowl, Poke Ball Cushion, Potted Plant,
  // Toy Chest, Poke Ball Toy, Small Wall Poster. Seven free owned items.
  const uint8_t starters[] = {1, 10, 62, 11, 63, 64, 61};
  for (uint8_t id : starters) unlock(id);
  // The seven starter decorations are owned but all are OFF initially.
  // Players choose which ones to display in the habitat editor.
}
bool DecorState::owns(uint8_t id) const { return decorValid(id) && bitGet(owned_, id); }
bool DecorState::discovered(uint8_t id) const { return decorValid(id) && bitGet(discovered_, id); }
bool DecorState::unlock(uint8_t id) {
  if (!decorValid(id)) return false;
  const bool fresh = !owns(id);
  bitSet(owned_, id); bitSet(discovered_, id);
  return fresh;
}
uint8_t DecorState::placed(uint8_t slot) const { return slot < DECOR_SLOTS ? slots_[slot] : 0; }
uint8_t DecorState::placedCount(uint8_t category) const {
  if (category >= DC_COUNT) return 0;
  uint8_t count = 0;
  for (uint8_t id : slots_) if (id && decorDef(id).category == category) ++count;
  return count;
}
bool DecorState::validLayout() const {
  uint8_t totals[DC_COUNT] = {};
  uint8_t seen[DECOR_FLAGS_BYTES] = {};
  for (uint8_t id : slots_) {
    if (!id) continue;
    if (!decorValid(id) || !owns(id) || bitGet(seen, id)) return false;
    bitSet(seen, id);
    if (++totals[decorDef(id).category] > DECOR_LIMITS[decorDef(id).category]) return false;
  }
  return true;
}
bool DecorState::place(uint8_t slot, uint8_t id) {
  if (slot >= DECOR_SLOTS || (id && !owns(id))) return false;
  // v1: one placed instance per owned item. No duplicates or overlaps.
  if (id) for (uint8_t i = 0; i < DECOR_SLOTS; ++i)
    if (i != slot && slots_[i] == id) return false;
  const uint8_t old = slots_[slot];
  slots_[slot] = id;
  if (!validLayout()) { slots_[slot] = old; return false; }
  return true;
}
uint8_t DecorState::habitatCount() const {
  uint8_t count = 0;
  for (uint8_t id : slots_) if (id) ++count;
  return count;
}

bool DecorState::normalizeHabitat() {
  bool changed = false;
  uint8_t seen[DC_COUNT] = {};
  uint8_t total = 0;
  for (uint8_t i = 0; i < DECOR_SLOTS; ++i) {
    const uint8_t id = slots_[i];
    if (!id) continue;
    const uint8_t cat = decorDef(id).category;
    if (seen[cat] || total >= 4) {
      slots_[i] = 0;
      changed = true;
    } else {
      ++seen[cat];
      ++total;
    }
  }
  return changed;
}

bool DecorState::toggleHabitat(uint8_t id) {
  if (!owns(id)) return false;
  const uint8_t category = decorDef(id).category;
  int oldSlot = -1;
  int freeSlot = -1;
  for (uint8_t i = 0; i < DECOR_SLOTS; ++i) {
    if (slots_[i] == id) { slots_[i] = 0; return true; }
    if (slots_[i] && decorDef(slots_[i]).category == category) oldSlot = i;
    if (!slots_[i] && freeSlot < 0) freeSlot = i;
  }
  // Replacing an equipped decoration of this category does not use another slot.
  if (oldSlot >= 0) {
    slots_[oldSlot] = id;
    return true;
  }
  if (habitatCount() >= 4 || freeSlot < 0) return false;
  slots_[freeSlot] = id;
  return true;
}

void DecorState::encode(uint8_t out[WIRE_BYTES]) const {
  memcpy(out, owned_, DECOR_FLAGS_BYTES);
  memcpy(out + DECOR_FLAGS_BYTES, discovered_, DECOR_FLAGS_BYTES);
  memcpy(out + 2 * DECOR_FLAGS_BYTES, slots_, DECOR_SLOTS);
}
bool DecorState::decode(const uint8_t *data, size_t size) {
  if (!data || size != WIRE_BYTES) return false;
  DecorState candidate;
  memcpy(candidate.owned_, data, DECOR_FLAGS_BYTES);
  memcpy(candidate.discovered_, data + DECOR_FLAGS_BYTES, DECOR_FLAGS_BYTES);
  memcpy(candidate.slots_, data + 2 * DECOR_FLAGS_BYTES, DECOR_SLOTS);
  if (!candidate.validLayout()) return false;
  // Never allow equipped/owned item without its discovery flag.
  for (uint8_t id = 1; id < DECOR_ITEM_COUNT; ++id)
    if (candidate.owns(id) && !candidate.discovered(id)) return false;
  *this = candidate;
  return true;
}
