#pragma once
#include <stdint.h>
#include <stddef.h>

// TamaToys decorations: persistent IDs are APPEND-ONLY. ID 0 means empty.
// Kept separate from items.h to preserve existing ItemKey/save compatibility.
enum DecorCategory : uint8_t {
  DC_REST, DC_CARE, DC_TOY, DC_NATURE, DC_FURNITURE,
  DC_ACCENT, DC_SPECIAL, DC_COUNT
};
constexpr uint8_t DECOR_LIMITS[DC_COUNT] = {1, 1, 2, 2, 2, 2, 1};
constexpr uint8_t DECOR_SLOTS = 11; // sum of category caps
constexpr uint8_t DECOR_ITEM_COUNT = 65; // includes the reserved empty ID
constexpr uint8_t DECOR_FLAGS_BYTES = (DECOR_ITEM_COUNT + 7) / 8;

struct DecorDef {
  const char *name;
  uint8_t category;
  uint16_t price; // 0 = discoverable-only, not sold at Mart
};

// A permanent item ID. Put new items at the end, never insert/renumber.
static const DecorDef DECOR_TABLE[DECOR_ITEM_COUNT] = {
  {"-", DC_ACCENT, 0},
  {"Small Bed", DC_REST, 250},
  {"Deluxe Bed", DC_REST, 900},
  {"Beanbag Chair", DC_FURNITURE, 300},
  {"Wooden Table", DC_FURNITURE, 350},
  {"Couch", DC_FURNITURE, 850},
  {"Bookshelf", DC_FURNITURE, 600},
  {"Toy Chest", DC_FURNITURE, 450},
  {"Floor Lamp", DC_ACCENT, 275},
  {"Rocking Chair", DC_FURNITURE, 550},
  {"Food Bowl", DC_CARE, 150},
  {"Potted Plant", DC_NATURE, 200},
  {"Flower Pot", DC_NATURE, 180},
  {"Wall Clock", DC_ACCENT, 300},
  {"Picture Frame", DC_ACCENT, 225},
  {"Round Rug", DC_ACCENT, 350},
  {"Star Rug", DC_ACCENT, 500},
  {"Poke Ball Cushion", DC_TOY, 300},
  {"Curtains", DC_ACCENT, 275},
  {"Aquarium", DC_FURNITURE, 1100},
  {"Mini Fountain", DC_NATURE, 1250},
  {"Oak Tree", DC_NATURE, 500},
  {"Pine Tree", DC_NATURE, 500},
  {"Berry Bush", DC_NATURE, 250},
  {"Flower Patch", DC_NATURE, 175},
  {"Mushroom Cluster", DC_NATURE, 200},
  {"Large Rock", DC_NATURE, 300},
  {"Tree Stump", DC_NATURE, 225},
  {"Log Bench", DC_FURNITURE, 375},
  {"Small Pond", DC_NATURE, 1200},
  {"Wooden Fence", DC_ACCENT, 200},
  {"Camping Tent", DC_REST, 850},
  {"Campfire", DC_ACCENT, 400},
  {"Picnic Blanket", DC_ACCENT, 350},
  {"Beach Umbrella", DC_ACCENT, 650},
  {"Beach Chair", DC_FURNITURE, 475},
  {"Sandcastle", DC_NATURE, 275},
  {"Hammock", DC_REST, 800},
  {"Lantern", DC_ACCENT, 225},
  {"Park Bench", DC_FURNITURE, 450},
  {"Birdbath", DC_NATURE, 650},
  {"Poke Ball Toy", DC_TOY, 175},
  {"Rubber Ball", DC_TOY, 125},
  {"Teddy Bear", DC_TOY, 250},
  {"Building Blocks", DC_TOY, 200},
  {"Jump Rope", DC_TOY, 150},
  {"Toy Train", DC_TOY, 450},
  {"Mini Slide", DC_TOY, 800},
  {"Swing", DC_TOY, 950},
  {"Scratch Post", DC_TOY, 375},
  {"Bubble Machine", DC_TOY, 900},
  {"Crystal Lamp", DC_SPECIAL, 0},
  {"Golden Statue", DC_SPECIAL, 0},
  {"Meteorite", DC_SPECIAL, 0},
  {"Ancient Relic", DC_SPECIAL, 0},
  {"Glowing Mushroom", DC_SPECIAL, 0},
  {"Mini Waterfall", DC_SPECIAL, 0},
  {"Volcano Model", DC_SPECIAL, 0},
  {"Snow Globe", DC_SPECIAL, 0},
  {"Treasure Chest", DC_SPECIAL, 0},
  {"Mystery Egg Statue", DC_SPECIAL, 0},
  {"Small Wall Poster", DC_ACCENT, 125},
  {"Small Cushion", DC_TOY, 150},
  {"Wooden Crate", DC_FURNITURE, 175},
  {"Basic Toy Ball", DC_TOY, 100},
};
static_assert(sizeof(DECOR_TABLE) / sizeof(DECOR_TABLE[0]) == DECOR_ITEM_COUNT,
              "Decor catalog count does not match");

inline bool decorValid(uint8_t id) { return id > 0 && id < DECOR_ITEM_COUNT; }
inline const DecorDef &decorDef(uint8_t id) { return DECOR_TABLE[decorValid(id) ? id : 0]; }
inline bool decorForSale(uint8_t id) { return decorValid(id) && decorDef(id).price != 0; }

// Pure game logic; no Arduino dependencies, can be unit tested on desktop.
class DecorState {
public:
  DecorState();
  void resetToStarters();
  bool owns(uint8_t id) const;
  bool discovered(uint8_t id) const;
  bool unlock(uint8_t id); // expedition/shop reward: permanent ownership & discovery
  bool place(uint8_t slot, uint8_t id); // 0 = clear; category limits enforced
  uint8_t placed(uint8_t slot) const;
  uint8_t placedCount(uint8_t category) const;
  bool validLayout() const;
  // Habitat v1: at most one displayed item per category and four total.
  // Keeps the existing wire format and future two-per-category catalog caps.
  bool normalizeHabitat(); // drops hidden legacy placements; true if changed
  bool toggleHabitat(uint8_t id); // ON->OFF, OFF->replace same category, or fail if full
  uint8_t habitatCount() const;
  // Stable, explicit payload. Bits + 11 slot IDs; independent of compiler padding.
  static constexpr size_t WIRE_BYTES = DECOR_FLAGS_BYTES * 2 + DECOR_SLOTS;
  void encode(uint8_t out[WIRE_BYTES]) const;
  bool decode(const uint8_t *data, size_t size);
private:
  uint8_t owned_[DECOR_FLAGS_BYTES];
  uint8_t discovered_[DECOR_FLAGS_BYTES];
  uint8_t slots_[DECOR_SLOTS];
};
