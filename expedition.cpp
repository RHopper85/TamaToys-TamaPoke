#include "expedition.h"
#include "decor_store.h"
#include <string.h>

ExpeditionManager expeditions;
namespace {
constexpr uint32_t MAGIC = 0x31505845u; // EXP1
constexpr uint16_t VERSION = 1;
constexpr size_t HEADER = 16;
constexpr size_t PAYLOAD = 16 + sizeof(PartyMon) + 1 + sizeof(ExpeditionJournalEntry) * 8;
constexpr size_t CAP = HEADER + PAYLOAD + 2;
uint16_t crc16(const uint8_t *p, size_t n) {
  uint16_t c=0xffff;
  for (size_t i=0;i<n;i++) { c^=uint16_t(p[i])<<8;
    for (int j=0;j<8;j++) c=(c&0x8000)?uint16_t((c<<1)^0x1021):uint16_t(c<<1);
  }
  return c;
}
void put16(uint8_t *p,uint16_t v) { memcpy(p,&v,2); }
void put32(uint8_t *p,uint32_t v) { memcpy(p,&v,4); }
uint16_t rd16(const uint8_t *p) { uint16_t v;memcpy(&v,p,2);return v; }
uint32_t rd32(const uint8_t *p) { uint32_t v;memcpy(&v,p,4);return v; }
uint16_t readSlot(Preferences &p,const char *key,uint8_t *buf) {
  size_t n=p.getBytesLength(key);
  if (n<HEADER+2 || n>CAP || n<HEADER+16+1+2) return 0;
  if (p.getBytes(key,buf,n)!=n || rd32(buf)!=MAGIC || rd16(buf+4)!=VERSION || rd16(buf+6)!=n) return 0;
  if (rd16(buf+n-2)!=crc16(buf,n-2)) return 0;
  return uint16_t(n);
}
}

void ExpeditionManager::resetRAM() {
  state_=0; fromBox_=false; slot_=0; destination_=0; startEpoch_=seed_=generation_=0;
  traveler_=PartyMon(); journalCount_=0; for (auto &entry : history_) entry = ExpeditionJournalEntry();
}

bool ExpeditionManager::sameMon(const PartyMon &a,const PartyMon &b) const {
  return !a.empty() && a.dex==b.dex && a.level==b.level && a.shiny==b.shiny &&
    a.ageMinutes==b.ageMinutes && !strncmp(a.nick,b.nick,sizeof(a.nick));
}

bool ExpeditionManager::persist() {
  if (!healthy_) return false;
  uint8_t buf[CAP]={};
  const uint32_t next=generation_+1;
  put32(buf,MAGIC);put16(buf+4,VERSION);put16(buf+6,uint16_t(CAP));
  put32(buf+8,next);put16(buf+12,sizeof(PartyMon));put16(buf+14,8);
  size_t at=HEADER;
  buf[at++]=state_;buf[at++]=fromBox_?1:0;buf[at++]=slot_;buf[at++]=destination_;
  put32(buf+at,startEpoch_);at+=4;put32(buf+at,seed_);at+=4;
  memset(buf+at,0,4);at+=4;
  memcpy(buf+at,&traveler_,sizeof(traveler_));at+=sizeof(traveler_);
  buf[at++]=journalCount_;memcpy(buf+at,history_,sizeof(history_));at+=sizeof(history_);
  put16(buf+at,crc16(buf,at));
  const char *key=(next&1)?"expA":"expB";
  if (prefs_.putBytes(key,buf,CAP)!=CAP) return false;
  uint8_t check[CAP];
  if (readSlot(prefs_,key,check)!=CAP || memcmp(check,buf,CAP)) return false;
  generation_=next;return true;
}

bool ExpeditionManager::restore() {
  uint8_t a[CAP],b[CAP];
  uint16_t na=readSlot(prefs_,"expA",a),nb=readSlot(prefs_,"expB",b);
  if (!na && !nb) return false;
  const uint8_t *p=na?a:b;
  if (nb && (!na || int32_t(rd32(b+8)-rd32(a+8))>0)) p=b;
  // Reject incompatible PartyMon layouts rather than misinterpreting saved monsters.
  if (rd16(p+12)!=sizeof(PartyMon) || rd16(p+14)!=8 || rd16(p+6)!=CAP) return false;
  generation_=rd32(p+8);
  size_t at=HEADER;
  state_=p[at++];fromBox_=p[at++]!=0;slot_=p[at++];destination_=p[at++];
  startEpoch_=rd32(p+at);at+=4;seed_=rd32(p+at);at+=8;
  memcpy(&traveler_,p+at,sizeof(traveler_));at+=sizeof(traveler_);
  journalCount_=p[at++];memcpy(history_,p+at,sizeof(history_));
  if (journalCount_>8 || state_>2 || destination_>=EXP_DEST_COUNT ||
      (state_ && (traveler_.empty() || slot_ >= (fromBox_?BOX_SLOTS:PARTY_SLOTS)))) return false;
  return true;
}

void ExpeditionManager::reconcileDispatch() {
  if (!state_ || traveler_.empty()) return;
  if (state_ == 2) {
    // Return was interrupted. Restore exactly once before clearing the escrow.
    bool present = false;
    for (uint8_t i=0;i<PARTY_SLOTS;i++) if (sameMon(party.slots[i],traveler_)) present=true;
    for (uint8_t i=0;i<BOX_SLOTS;i++) if (sameMon(party.box[i],traveler_)) present=true;
    if (!present) {
      if (!party.add(traveler_) && !party.boxAdd(traveler_)) return;
    }
    state_=0;
    persist();
    return;
  }
  PartyMon &source=fromBox_?party.box[slot_]:party.slots[slot_];
  if (sameMon(source,traveler_)) {
    if (fromBox_) party.boxReleaseAt(slot_); else party.releaseAt(slot_);
  }
}

bool ExpeditionManager::begin() {
  resetRAM();
  healthy_=prefs_.begin("tamapoke",false);
  if (!healthy_) return false;
  if (restore()) { reconcileDispatch(); return true; }
  if (prefs_.isKey("expA") || prefs_.isKey("expB")) {
    healthy_=false; Serial.println("expedition: saved records unreadable; refusing overwrite");return false;
  }
  return persist();
}

bool ExpeditionManager::dispatch(bool fromBox,uint8_t slot,uint32_t epoch,uint8_t dest,bool livePet) {
  if (!healthy_ || !epoch || dest>=EXP_DEST_COUNT || state_) return false;
  uint8_t available=livePet?1:0;
  for (uint8_t i=0;i<PARTY_SLOTS;i++) if (!party.slots[i].empty()) available++;
  for (uint8_t i=0;i<BOX_SLOTS;i++) if (!party.box[i].empty()) available++;
  if (!expeditionEligible(available,livePet,state_!=0)) return false;
  if (slot>=(fromBox?BOX_SLOTS:PARTY_SLOTS)) return false;
  const PartyMon mon=fromBox?party.box[slot]:party.slots[slot];
  if (mon.empty()) return false;
  // Commit an escrow copy BEFORE removing the mon from its original slot.
  // On a reboot reconcileDispatch() finishes the removal.
  state_=1;fromBox_=fromBox;slot_=slot;destination_=dest;
  traveler_=mon;startEpoch_=epoch;seed_=epoch ^ (uint32_t(mon.dex)<<16) ^ (uint32_t(mon.level)<<3) ^ uint32_t(millis());
  if (!persist()) {state_=0;traveler_=PartyMon();return false;}
  if (fromBox) party.boxReleaseAt(slot); else party.releaseAt(slot);
  return true;
}

uint32_t ExpeditionManager::remainingSeconds(uint32_t epoch) const {
  if (!state_) return 0;
  uint32_t duration=uint32_t(EXP_DESTS[destination_].minutes)*60u;
  if (epoch<startEpoch_) return duration;
  uint32_t elapsed=epoch-startEpoch_;
  return elapsed>=duration?0:duration-elapsed;
}
bool ExpeditionManager::isReady(uint32_t epoch) const { return state_ && epoch && !remainingSeconds(epoch); }
ExpeditionJournalEntry ExpeditionManager::journal(uint8_t i) const {
  return i<journalCount_?history_[i]:ExpeditionJournalEntry();
}

bool ExpeditionManager::claim(uint32_t epoch,uint32_t &coins,uint8_t &decoration,ExpeditionOutcome &outcome) {
  coins=0;decoration=0;
  if (!healthy_ || !isReady(epoch)) return false;
  // Save returned Pokemon before awarding prizes. If storage is full, keep
  // expedition locked rather than lose the Pokemon.
  if (party.firstFree()<0 && party.boxFirstFree()<0) return false;
  outcome=expeditionRoll(seed_,EXP_DESTS[destination_].difficulty,traveler_.level);
  coins=expeditionCoins(EXP_DESTS[destination_].difficulty,outcome,seed_);
  decoration=expeditionDecoration(destination_,outcome,seed_);
  if (decoration && !decorSaveHealthy()) return false;
  // Durable RETURNING state first. On boot, reconcileDispatch() restores the
  // escrowed Pokemon once, even if power was interrupted between writes.
  state_=2;
  if (!persist()) {state_=1;return false;}
  if (!party.add(traveler_)) party.boxAdd(traveler_);
  state_=0;
  if (!persist()) {
    // Pokemon is already home, and the checkpoint still says RETURNING.
    // A reboot will reconcile without making a duplicate.
    healthy_=false;
    return false;
  }
  if (decoration && !decor.owns(decoration)) {
    DecorState backup=decor;
    if (!decor.unlock(decoration) || !decorSave()) { decor=backup; decoration=0; }
  }
  // Wallet is managed by the existing Pet checkpoint; caller awards coins.
  ExpeditionJournalEntry entry;
  entry.destination=destination_;entry.outcome=outcome;entry.coins=uint16_t(coins);entry.decoration=decoration;
  for(int i=7;i>0;i--) history_[i]=history_[i-1];
  history_[0]=entry;
  if(journalCount_<8)journalCount_++;
  traveler_=PartyMon();
  persist();
  return true;
}
