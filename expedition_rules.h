#pragma once
#include <stdint.h>
#include "items.h"

// Destination IDs and decoration IDs are persistent: append, never reorder.
struct ExpeditionDestination {
  const char *name;
  uint16_t minutes;
  uint8_t difficulty;
  const char *preferredType;
  uint8_t commonDecor; // Exclusive to this destination; unavailable in the Mart.
  uint8_t rareDecor;   // 0 when this destination has one exclusive collectible.
};
static const ExpeditionDestination EXP_DESTS[] = {
  {"VIRIDIAN FOREST", 15, 1, "GRASS", 55, 0}, // Glowing Mushroom
  {"ROCKY CAVE",      30, 1, "ROCK", 57, 53}, // Volcano Model, Meteorite
  {"SUNNY BEACH",     45, 2, "WATER", 56, 0}, // Mini Waterfall
  {"HAUNTED WOODS",   60, 2, "GHOST", 60, 0}, // Mystery Egg Statue
  {"CRYSTAL CAVERN", 120, 3, "ROCK", 51, 59}, // Crystal Lamp, Treasure Chest
  {"SNOWPEAK",       180, 3, "ICE", 58, 0}, // Snow Globe
  {"ANCIENT RUINS",  240, 4, "PSYCHIC", 54, 52}, // Ancient Relic, Golden Statue
};
constexpr uint8_t EXP_DEST_COUNT = sizeof(EXP_DESTS) / sizeof(EXP_DESTS[0]);
enum ExpeditionOutcome : uint8_t { EXP_SUCCESS=1, EXP_PARTIAL=2, EXP_FAILED=3 };

struct ExpeditionItemReward { ItemKey key=ITEM_NONE; uint8_t count=0; };
struct ExpeditionRewards {
  uint32_t coins=0;
  uint8_t decoration=0;
  uint8_t itemCount=0;
  ExpeditionItemReward items[3];
};

inline uint32_t expMix(uint32_t seed, uint32_t domain) {
  uint32_t x=(seed ^ domain) + 0x9e3779b9u;
  x ^= x >> 16; x *= 0x7feb352du;
  x ^= x >> 15; x *= 0x846ca68bu;
  x ^= x >> 16; return x;
}
inline bool expeditionEligible(uint8_t available,bool livePet,bool busy) {
  return livePet && available>=2 && !busy;
}
inline uint8_t expeditionSuccessChance(uint8_t difficulty,uint16_t level,bool typeMatch=false) {
  int p=77-int(difficulty)*13+int(level/8)+(typeMatch?10:0);
  if(p<22)p=22;
  if(p>90)p=90;
  return static_cast<uint8_t>(p);
}
inline ExpeditionOutcome expeditionRoll(uint32_t seed,uint8_t difficulty,uint16_t level,bool typeMatch=false) {
  const uint8_t r=static_cast<uint8_t>(expMix(seed ^ (uint32_t(level)*2654435761u),0xC3F70B1u ^ difficulty)%100u);
  const int s=expeditionSuccessChance(difficulty,level,typeMatch);
  if(r<s)return EXP_SUCCESS;
  if(r<s+25)return EXP_PARTIAL;
  return EXP_FAILED;
}
inline uint16_t expeditionCoins(uint8_t difficulty,ExpeditionOutcome outcome,uint32_t seed) {
  if(outcome==EXP_FAILED)return 0;
  uint16_t v=static_cast<uint16_t>(difficulty*70 + (expMix(seed,0xABCD12u)%61));
  return outcome==EXP_PARTIAL?static_cast<uint16_t>(v/3):v;
}
inline bool expeditionFinished(uint32_t start,uint32_t now,uint16_t minutes) {
  return start && now>=start && (now-start)>=uint32_t(minutes)*60u;
}
inline uint8_t expeditionDecoration(uint8_t destination,ExpeditionOutcome outcome,uint32_t seed) {
  if(destination>=EXP_DEST_COUNT||outcome==EXP_FAILED)return 0;
  // Exactly 5% on success and 1% on partial, independent of difficulty.
  const uint32_t chance=outcome==EXP_SUCCESS?5u:1u;
  if(expMix(seed,0xDEC0A12u)%100u>=chance)return 0;
  const auto &d=EXP_DESTS[destination];
  return d.rareDecor && (expMix(seed,0x1A2B3C4Du)%100u)<15u?d.rareDecor:d.commonDecor;
}
inline ExpeditionRewards expeditionRewardPlan(uint8_t destination,ExpeditionOutcome outcome,uint32_t seed) {
  ExpeditionRewards result;
  if(destination>=EXP_DEST_COUNT || outcome==EXP_FAILED)return result;
  const auto &d=EXP_DESTS[destination];
  result.coins=expeditionCoins(d.difficulty,outcome,seed);
  result.decoration=expeditionDecoration(destination,outcome,seed);
  // Existing consumable keys only. Each region has a distinct two/three-item pool.
  static const ItemKey pools[EXP_DEST_COUNT][3]={
    {IT_POKEBALL,IT_POTION,IT_GREATBALL},
    {IT_GREATBALL,IT_POTION,IT_SUPERPOTION},
    {IT_POTION,IT_POKEBALL,IT_FULLHEAL},
    {IT_FULLHEAL,IT_GREATBALL,IT_POTION},
    {IT_GREATBALL,IT_SUPERPOTION,IT_ULTRABALL},
    {IT_SUPERPOTION,IT_FULLHEAL,IT_GREATBALL},
    {IT_ULTRABALL,IT_HYPERPOTION,IT_GREATBALL}
  };
  result.itemCount=outcome==EXP_PARTIAL?
    static_cast<uint8_t>(expMix(seed,0xB00B05u)%2u):
    static_cast<uint8_t>(1u+expMix(seed,0xB00B05u)%3u);
  for(uint8_t i=0;i<result.itemCount;i++) {
    result.items[i].key=pools[destination][expMix(seed,0x153u+i*997u)%3u];
    result.items[i].count=static_cast<uint8_t>(1u+expMix(seed,0x3A77u+i*619u)%3u);
  }
  return result;
}
