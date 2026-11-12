#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "party.h"
#include "expedition_rules.h"

struct ExpeditionJournalEntry {
  uint8_t destination=0;
  uint8_t outcome=0;
  uint8_t decoration=0;
  uint16_t coins=0;
};
class ExpeditionManager {
public:
  bool begin();
  bool healthy() const {return healthy_;}
  bool active() const {return state_!=0;}
  bool isReady(uint32_t epoch) const;
  uint8_t destination() const {return destination_;}
  const PartyMon &traveler() const {return traveler_;} // escrow snapshot for the UI
  uint32_t startedAt() const {return startEpoch_;}
  uint32_t remainingSeconds(uint32_t epoch) const;
  uint8_t journalCount() const {return journalCount_;}
  ExpeditionJournalEntry journal(uint8_t i) const;
  bool dispatch(bool fromBox,uint8_t slot,uint32_t epoch,uint8_t dest,bool livePet);
  // Claim includes item, coin, decor and Pokemon delivery; can resume after a reset.
  bool claim(uint32_t epoch,ExpeditionRewards &reward,ExpeditionOutcome &outcome);
  void resetRAM();
private:
  bool persist();
  bool restore();
  bool settle();
  void reconcileDispatch();
  bool sameMon(const PartyMon &a,const PartyMon &b) const;
  bool healthy_=false;
  uint8_t state_=0; // 0 idle, 1 traveling, 2 reward delivery pending
  bool fromBox_=false;
  uint8_t slot_=0;
  uint8_t destination_=0;
  uint32_t startEpoch_=0,seed_=0,generation_=0;
  PartyMon traveler_;
  ExpeditionJournalEntry history_[8];
  uint8_t journalCount_=0;
  ExpeditionRewards pending_;
  uint32_t walletTarget_=0;
  uint8_t itemTargets_[3]={};
  bool returnFromBox_=false;
  uint8_t returnSlot_=0;
  ExpeditionOutcome pendingOutcome_=EXP_FAILED;
  Preferences prefs_;
};
extern ExpeditionManager expeditions;
