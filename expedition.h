#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "party.h"
#include "expedition_rules.h"

struct ExpeditionJournalEntry {
  uint8_t destination = 0;
  uint8_t outcome = 0;
  uint8_t decoration = 0;
  uint16_t coins = 0;
};

class ExpeditionManager {
public:
  bool begin();
  bool healthy() const { return healthy_; }
  bool active() const { return state_ != 0; }
  bool isReady(uint32_t epoch) const;
  uint8_t destination() const { return destination_; }
  uint32_t startedAt() const { return startEpoch_; }
  uint32_t remainingSeconds(uint32_t epoch) const;
  uint8_t journalCount() const { return journalCount_; }
  ExpeditionJournalEntry journal(uint8_t i) const;
  // origin: false = party, true = box. A live pet must always stay home.
  bool dispatch(bool fromBox, uint8_t slot, uint32_t epoch, uint8_t dest, bool livePet);
  // Claims are user initiated; an unfinished expedition cannot be recalled.
  bool claim(uint32_t epoch, uint32_t &coins, uint8_t &decoration, ExpeditionOutcome &outcome);
  void resetRAM(); // used only by tests; Factory Reset clears tamapoke NVS.
private:
  bool persist();
  bool restore();
  void reconcileDispatch();
  bool sameMon(const PartyMon &a, const PartyMon &b) const;
  bool healthy_ = false;
  uint8_t state_ = 0; // 0 idle, 1 in expedition, 2 result awaiting collection
  bool fromBox_ = false;
  uint8_t slot_ = 0;
  uint8_t destination_ = 0;
  uint32_t startEpoch_ = 0;
  uint32_t seed_ = 0;
  uint32_t generation_ = 0;
  PartyMon traveler_;
  ExpeditionJournalEntry history_[8];
  uint8_t journalCount_ = 0;
  Preferences prefs_;
};
extern ExpeditionManager expeditions;
