#include "decor_store.h"
#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

DecorState decor;
static uint32_t decorGeneration = 0;
static bool decorHealthy = false;
static bool decorBlocked = false;

// Explicit little-endian wire format; no dependence on struct padding.
// 4 magic, 1 version, 1 length, 4 generation, WIRE_BYTES payload, 2 CRC.
static constexpr uint8_t FORMAT = 1;
static constexpr size_t HEADER = 10;
static constexpr size_t RECORD = HEADER + DecorState::WIRE_BYTES + 2;
static constexpr uint8_t MAGIC[4] = {'T', 'D', 'E', 'C'};
static_assert(RECORD < 256, "decoration record length no longer fits u8");

static uint16_t crc16(const uint8_t *p, size_t n) {
  uint16_t c = 0xFFFF;
  for (size_t i = 0; i < n; ++i) {
    c ^= (uint16_t)p[i] << 8;
    for (int b = 0; b < 8; ++b)
      c = (c & 0x8000) ? (uint16_t)((c << 1) ^ 0x1021) : (uint16_t)(c << 1);
  }
  return c;
}
static uint32_t rd32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
         (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void wr32(uint8_t *p, uint32_t v) {
  for (int i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8 * i));
}
static bool newer(uint32_t a, uint32_t b) { return (int32_t)(a - b) > 0; }

static bool readSlot(Preferences &p, const char *key, DecorState &out, uint32_t &gen) {
  if (p.getBytesLength(key) != RECORD) return false;
  uint8_t b[RECORD];
  if (p.getBytes(key, b, sizeof(b)) != sizeof(b)) return false;
  if (memcmp(b, MAGIC, 4) || b[4] != FORMAT || b[5] != RECORD) return false;
  uint16_t expected = (uint16_t)b[RECORD - 2] | (uint16_t)b[RECORD - 1] << 8;
  if (expected != crc16(b, RECORD - 2)) return false;
  DecorState candidate;
  if (!candidate.decode(b + HEADER, DecorState::WIRE_BYTES)) return false;
  out = candidate;
  gen = rd32(b + 6);
  return true;
}

bool decorSave() {
  if (decorBlocked) return false;
  Preferences p;
  if (!p.begin("tamapoke", false)) { decorHealthy = false; return false; }
  uint32_t next = decorGeneration + 1;
  const char *key = (next & 1) ? "decA" : "decB";
  uint8_t b[RECORD] = {};
  memcpy(b, MAGIC, 4);
  b[4] = FORMAT;
  b[5] = (uint8_t)RECORD;
  wr32(b + 6, next);
  decor.encode(b + HEADER);
  uint16_t c = crc16(b, RECORD - 2);
  b[RECORD - 2] = (uint8_t)c;
  b[RECORD - 1] = (uint8_t)(c >> 8);
  bool ok = p.putBytes(key, b, RECORD) == RECORD;
  if (ok) {
    DecorState verify;
    uint32_t verifyGen = 0;
    ok = readSlot(p, key, verify, verifyGen) && verifyGen == next;
  }
  p.end();
  if (ok) decorGeneration = next;
  decorHealthy = ok;
  if (!ok) Serial.println("decor: checkpoint save failed; previous slot preserved");
  return ok;
}

bool decorBegin() {
  decor = DecorState();
  decorGeneration = 0;
  decorHealthy = false;
  decorBlocked = false;
  Preferences p;
  if (!p.begin("tamapoke", false)) {
    decorBlocked = true;
    Serial.println("decor: NVS unavailable; starter inventory in RAM only");
    return false;
  }
  const bool anySlot = p.isKey("decA") || p.isKey("decB");
  DecorState a, b;
  uint32_t ga = 0, gb = 0;
  const bool va = readSlot(p, "decA", a, ga);
  const bool vb = readSlot(p, "decB", b, gb);
  p.end();
  if (va || vb) {
    if (va && (!vb || newer(ga, gb))) { decor = a; decorGeneration = ga; }
    else { decor = b; decorGeneration = gb; }
    decorHealthy = true;
    return true;
  }
  if (anySlot) {
    // Do not replace an existing save with starters after an unreadable record.
    decorBlocked = true;
    Serial.println("decor: both checkpoints unreadable; refusing overwrite");
    return false;
  }
  // First boot, including New Game / Factory Reset when game NVS was cleared.
  return decorSave();
}

bool decorSaveHealthy() { return decorHealthy && !decorBlocked; }
