#pragma once
#include <stdint.h>

// Durable destination IDs. APPEND new destinations; never reorder saved IDs.
struct ExpeditionDestination {
  const char* name;
  uint16_t minutes;
  uint8_t difficulty;
  uint8_t preferredType; // informational in v1; type bonuses are future work
  uint8_t commonDecor;
  uint8_t rareDecor;
};
static const ExpeditionDestination EXP_DESTS[] = {
  {"VIRIDIAN FOREST", 15, 1, 12, 24, 55},
  {"ROCKY CAVE", 30, 1, 5, 26, 53},
  {"SUNNY BEACH", 45, 2, 11, 36, 60},
  {"HAUNTED WOODS", 60, 2, 7, 25, 55},
  {"CRYSTAL CAVERN", 120, 3, 5, 27, 51},
  {"SNOWPEAK", 180, 3, 15, 16, 58},
  {"ANCIENT RUINS", 240, 4, 14, 23, 54},
};
constexpr uint8_t EXP_DEST_COUNT = sizeof(EXP_DESTS) / sizeof(EXP_DESTS[0]);
enum ExpeditionOutcome : uint8_t { EXP_SUCCESS=1, EXP_PARTIAL=2, EXP_FAILED=3 };
inline bool expeditionEligible(uint8_t available, bool livePet, bool busy) {
  // Dispatching a stored Pokemon requires a live Pokemon to remain home.
  return livePet && available >= 2 && !busy;
}
inline ExpeditionOutcome expeditionRoll(uint32_t seed, uint8_t difficulty, uint16_t level) {
  // One deterministic roll, permanently sealed in the checkpoint at dispatch.
  uint32_t x = seed ^ (uint32_t(level) * 2654435761u) ^ (uint32_t(difficulty) << 24);
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  uint8_t r = static_cast<uint8_t>(x % 100u);
  int success = 77 - int(difficulty) * 13 + int(level / 8);
  if (success > 90) success = 90;
  if (success < 22) success = 22;
  if (r < success) return EXP_SUCCESS;
  if (r < success + 25) return EXP_PARTIAL;
  return EXP_FAILED;
}
inline uint16_t expeditionCoins(uint8_t difficulty, ExpeditionOutcome outcome, uint32_t seed) {
  if (outcome == EXP_FAILED) return 0;
  uint16_t value = static_cast<uint16_t>(difficulty * 70 + (seed % 61));
  return outcome == EXP_PARTIAL ? static_cast<uint16_t>(value / 3) : value;
}
inline bool expeditionRareDrop(ExpeditionOutcome outcome, uint8_t difficulty, uint32_t seed) {
  return outcome == EXP_SUCCESS && ((seed >> 9) % 100u) < uint32_t(6 + difficulty * 3);
}
inline bool expeditionFinished(uint32_t start, uint32_t now, uint16_t minutes) {
  return start && now >= start && (now - start) >= (uint32_t)minutes * 60u;
}

inline uint8_t expeditionDecoration(uint8_t destination, ExpeditionOutcome outcome, uint32_t seed) {
  if (destination >= EXP_DEST_COUNT || outcome == EXP_FAILED) return 0;
  const auto &d = EXP_DESTS[destination];
  if (expeditionRareDrop(outcome,d.difficulty,seed)) return d.rareDecor;
  uint8_t chance = outcome == EXP_SUCCESS ? 48 : 12;
  return ((seed >> 17) % 100u) < chance ? d.commonDecor : 0;
}
