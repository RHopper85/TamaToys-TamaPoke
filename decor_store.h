#pragma once
#include "decor_catalog.h"

// Coordinates for stable habitat decoration anchors. Declared in a header
// so Arduino auto-generated .ino function prototypes can see the type.
struct HabitatPoint { int16_t x, y; };

// Habitat state is deliberately separate from Pet, Party and consumable Bag.
// decA / decB are alternating NVS checkpoints in the existing tamapoke namespace.
extern DecorState decor;

// Call once after pet.begin() on boot. An absent save grants starter decorations.
// Corrupt saves are never silently overwritten on boot.
bool decorBegin();
// Call after an owned/placed/discovery mutation. Returns false if not committed.
bool decorSave();
// True when a usable checkpoint was loaded or a fresh one was persisted.
bool decorSaveHealthy();
