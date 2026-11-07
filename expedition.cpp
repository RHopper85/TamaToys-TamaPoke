#include "expedition.h"
#include "decor_store.h"
#include "inventory.h"
#include "pet.h"
#include "dex.h"
#include "types.h"
#include <string.h>

extern Pet pet;
ExpeditionManager expeditions;
namespace {
constexpr uint32_t MAGIC=0x31505845u;
constexpr uint16_t V1=1,V2=2;
constexpr size_t HEADER=16;
constexpr size_t BASE=16+sizeof(PartyMon)+1+sizeof(ExpeditionJournalEntry)*8;
constexpr size_t OLD_CAP=HEADER+BASE+2;
constexpr size_t EXTRA=4+1+1+3+3+4+3+1+2; // coins, decor, count, keys, quantities, target wallet, bag targets, outcome
constexpr size_t CAP=OLD_CAP+EXTRA;
uint16_t crc16(const uint8_t*p,size_t n){
  uint16_t c=0xffff;
  for(size_t i=0;i<n;i++) {c^=uint16_t(p[i])<<8;
    for(int j=0;j<8;j++) c=(c&0x8000)?uint16_t((c<<1)^0x1021):uint16_t(c<<1);}
  return c;
}
void put16(uint8_t*p,uint16_t v){memcpy(p,&v,2);}
void put32(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
uint16_t rd16(const uint8_t*p){uint16_t v;memcpy(&v,p,2);return v;}
uint32_t rd32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
uint16_t readSlot(Preferences&p,const char*key,uint8_t*buf){
  size_t n=p.getBytesLength(key);
  if(n!=OLD_CAP && n!=CAP)return 0;
  if(p.getBytes(key,buf,n)!=n||rd32(buf)!=MAGIC||rd16(buf+6)!=n)return 0;
  if((rd16(buf+4)==V1&&n!=OLD_CAP)||(rd16(buf+4)==V2&&n!=CAP))return 0;
  if(rd16(buf+4)!=V1&&rd16(buf+4)!=V2)return 0;
  if(rd16(buf+n-2)!=crc16(buf,n-2))return 0;
  return uint16_t(n);
}
}
void ExpeditionManager::resetRAM(){
 state_=0;fromBox_=false;slot_=0;destination_=0;startEpoch_=seed_=generation_=0;
 traveler_=PartyMon();journalCount_=0;
 for(auto &e:history_)e=ExpeditionJournalEntry();
 pending_=ExpeditionRewards();walletTarget_=0;memset(itemTargets_,0,sizeof(itemTargets_));pendingOutcome_=EXP_FAILED;returnFromBox_=false;returnSlot_=0;
}
bool ExpeditionManager::sameMon(const PartyMon&a,const PartyMon&b) const {
 return !a.empty()&&a.dex==b.dex&&a.level==b.level&&a.shiny==b.shiny&&
 a.ageMinutes==b.ageMinutes&&!strncmp(a.nick,b.nick,sizeof(a.nick));
}
bool ExpeditionManager::persist(){
 if(!healthy_)return false;
 uint8_t buf[CAP]={};uint32_t next=generation_+1;
 put32(buf,MAGIC);put16(buf+4,V2);put16(buf+6,uint16_t(CAP));
 put32(buf+8,next);put16(buf+12,sizeof(PartyMon));put16(buf+14,8);
 size_t at=HEADER;
 buf[at++]=state_;buf[at++]=fromBox_?1:0;buf[at++]=slot_;buf[at++]=destination_;
 put32(buf+at,startEpoch_);at+=4;put32(buf+at,seed_);at+=4;at+=4;
 memcpy(buf+at,&traveler_,sizeof(traveler_));at+=sizeof(traveler_);
 buf[at++]=journalCount_;memcpy(buf+at,history_,sizeof(history_));at+=sizeof(history_);
 put32(buf+at,pending_.coins);at+=4;
 buf[at++]=pending_.decoration;buf[at++]=pending_.itemCount;
 for(int i=0;i<3;i++)buf[at++]=pending_.items[i].key;
 for(int i=0;i<3;i++)buf[at++]=pending_.items[i].count;
 put32(buf+at,walletTarget_);at+=4;
 for(int i=0;i<3;i++)buf[at++]=itemTargets_[i];
 buf[at++]=(uint8_t)pendingOutcome_;
 buf[at++]=returnFromBox_?1:0;buf[at++]=returnSlot_;
 if(at+2!=CAP)return false;
 put16(buf+at,crc16(buf,at));
 const char*key=(next&1)?"expA":"expB";
 if(prefs_.putBytes(key,buf,CAP)!=CAP)return false;
 uint8_t check[CAP]={};
 if(readSlot(prefs_,key,check)!=CAP||memcmp(check,buf,CAP))return false;
 generation_=next;return true;
}
bool ExpeditionManager::restore(){
 uint8_t a[CAP]={},b[CAP]={};
 uint16_t na=readSlot(prefs_,"expA",a),nb=readSlot(prefs_,"expB",b);
 if(!na&&!nb)return false;
 const uint8_t*p=na?a:b;
 if(nb&&(!na||int32_t(rd32(b+8)-rd32(a+8))>0))p=b;
 const uint16_t version=rd16(p+4);
 if(rd16(p+12)!=sizeof(PartyMon)||rd16(p+14)!=8)return false;
 generation_=rd32(p+8);
 size_t at=HEADER;
 state_=p[at++];fromBox_=p[at++]!=0;slot_=p[at++];destination_=p[at++];
 startEpoch_=rd32(p+at);at+=4;seed_=rd32(p+at);at+=8;
 memcpy(&traveler_,p+at,sizeof(traveler_));at+=sizeof(traveler_);
 journalCount_=p[at++];memcpy(history_,p+at,sizeof(history_));at+=sizeof(history_);
 if(version==V2){
   pending_.coins=rd32(p+at);at+=4;
   pending_.decoration=p[at++];pending_.itemCount=p[at++];
   for(int i=0;i<3;i++)pending_.items[i].key=p[at++];
   for(int i=0;i<3;i++)pending_.items[i].count=p[at++];
   walletTarget_=rd32(p+at);at+=4;
   for(int i=0;i<3;i++)itemTargets_[i]=p[at++];
   pendingOutcome_=(ExpeditionOutcome)p[at++];
   returnFromBox_=p[at++]!=0;returnSlot_=p[at++];
 }
 if(journalCount_>8||state_>2||destination_>=EXP_DEST_COUNT||pending_.itemCount>3||
    (version==V2&&state_==2&&returnSlot_>=(returnFromBox_?BOX_SLOTS:PARTY_SLOTS))||
    (state_&&(traveler_.empty()||slot_>=(fromBox_?BOX_SLOTS:PARTY_SLOTS))))return false;
 if(state_==2&&version==V1){
   // Legacy state 2 was returning, not a reward settlement. It did not
   // checkpoint a return slot. Prefer the original slot if free, otherwise
   // find a slot; do not re-award its old money/decoration.
   pending_=ExpeditionRewards();pendingOutcome_=EXP_FAILED;
   returnFromBox_=fromBox_;returnSlot_=slot_;
   PartyMon &old=returnFromBox_?party.box[returnSlot_]:party.slots[returnSlot_];
   if(!old.empty()&&!sameMon(old,traveler_)){
     int free=party.firstFree();
     returnFromBox_=free<0;
     returnSlot_=free>=0?(uint8_t)free:(uint8_t)party.boxFirstFree();
   }
 }
 return true;
}
bool ExpeditionManager::settle(){
 if(state_!=2||!healthy_)return false;
 // V2 reserves the return slot in the checkpoint before any delivery.
 // Looking for an identical Pokemon anywhere is insufficient: two legitimately
 // identical Pokemon must remain two separate records.
 PartyMon &target=returnFromBox_?party.box[returnSlot_]:party.slots[returnSlot_];
 if(!sameMon(target,traveler_)) {
   if(!target.empty())return false;
   bool inserted=returnFromBox_?party.boxAdd(traveler_):party.add(traveler_);
   if(!inserted||!sameMon(target,traveler_))return false;
 }
 // Monotonic absolute targets make replay after power loss idempotent.
 if(pet.wallet<walletTarget_){pet.wallet=walletTarget_;pet.saveNow();}
 for(uint8_t i=0;i<pending_.itemCount;i++){
   ItemKey k=pending_.items[i].key;
   if(!itemValid(k))continue;
   uint8_t have=bag.count(k);
   if(have<itemTargets_[i])bag.add(k,uint8_t(itemTargets_[i]-have));
 }
 if(pending_.decoration&&!decor.owns(pending_.decoration)){
   DecorState before=decor;
   decor.unlock(pending_.decoration);
   if(!decorSave()){decor=before;return false;}
 }
 // Check that item writes actually reached NVS before closing transaction.
 Preferences verify;
 if(!verify.begin("tamapoke",true))return false;
 uint8_t counts[ITEM_COUNT]={};
 const size_t stored=verify.getBytesLength("bag");
 bool bagOk=(pending_.itemCount==0);
 if(stored==sizeof(counts))bagOk=verify.getBytes("bag",counts,sizeof(counts))==sizeof(counts);
 verify.end();
 if(!bagOk)return false;
 for(uint8_t i=0;i<pending_.itemCount;i++){
   ItemKey k=pending_.items[i].key;
   if(itemValid(k)&&counts[k]<itemTargets_[i])return false;
 }
 if(!pet.saveHealthy())return false;
 state_=0;
 if(!persist()){state_=2;return false;}
 traveler_=PartyMon();return true;
}
void ExpeditionManager::reconcileDispatch(){
 if(!state_||traveler_.empty())return;
 if(state_==2){settle();return;}
 PartyMon &source=fromBox_?party.box[slot_]:party.slots[slot_];
 if(sameMon(source,traveler_)){
   if(fromBox_)party.boxReleaseAt(slot_);else party.releaseAt(slot_);
 }
}
bool ExpeditionManager::begin(){
 resetRAM();healthy_=prefs_.begin("tamapoke",false);
 if(!healthy_)return false;
 if(restore()){reconcileDispatch();return true;}
 if(prefs_.isKey("expA")||prefs_.isKey("expB")){
   healthy_=false;Serial.println("expedition: unreadable checkpoint; refusing overwrite");return false;
 }
 return persist();
}
bool ExpeditionManager::dispatch(bool fromBox,uint8_t slot,uint32_t epoch,uint8_t dest,bool livePet){
 if(!healthy_||!epoch||dest>=EXP_DEST_COUNT||state_)return false;
 uint8_t available=livePet?1:0;
 for(uint8_t i=0;i<PARTY_BENCH_SLOTS;i++)if(!party.slots[i].empty())available++;
 for(uint8_t i=0;i<BOX_SLOTS;i++)if(!party.box[i].empty())available++;
 if(!expeditionEligible(available,livePet,false))return false;
 if(slot>=(fromBox?BOX_SLOTS:PARTY_BENCH_SLOTS))return false;
 const PartyMon m=fromBox?party.box[slot]:party.slots[slot];
 if(m.empty())return false;
 state_=1;fromBox_=fromBox;slot_=slot;destination_=dest;traveler_=m;
 startEpoch_=epoch;seed_=epoch^(uint32_t(m.dex)<<16)^(uint32_t(m.level)<<3)^uint32_t(millis());
 pending_=ExpeditionRewards();walletTarget_=0;memset(itemTargets_,0,3);
 if(!persist()){state_=0;traveler_=PartyMon();return false;}
 if(fromBox)party.boxReleaseAt(slot);else party.releaseAt(slot);
 return true;
}
uint32_t ExpeditionManager::remainingSeconds(uint32_t epoch) const {
 if(!state_)return 0;
 uint32_t dur=uint32_t(EXP_DESTS[destination_].minutes)*60u;
 if(epoch<startEpoch_)return dur;
 uint32_t elapsed=epoch-startEpoch_;
 return elapsed>=dur?0:dur-elapsed;
}
bool ExpeditionManager::isReady(uint32_t epoch) const {return state_&&epoch&&!remainingSeconds(epoch);}
ExpeditionJournalEntry ExpeditionManager::journal(uint8_t i) const {
 return i<journalCount_?history_[i]:ExpeditionJournalEntry();
}
bool ExpeditionManager::claim(uint32_t epoch,ExpeditionRewards &reward,ExpeditionOutcome &outcome){
 reward=ExpeditionRewards();outcome=EXP_FAILED;
 if(!healthy_||!isReady(epoch))return false;
 if(state_==2){reward=pending_;outcome=pendingOutcome_;return settle();}
 int partySlot=party.firstFree(), boxSlot=party.boxFirstFree();
 if(partySlot<0&&boxSlot<0)return false;
 returnFromBox_=partySlot<0;
 returnSlot_=(uint8_t)(returnFromBox_?boxSlot:partySlot);
 const auto &dest=EXP_DESTS[destination_];
 const auto &species=DEX_TBL[traveler_.dex];
 const bool typeMatch=!strcmp(typeName(species.type1),dest.preferredType)||
    !strcmp(typeName(species.type2),dest.preferredType);
 pendingOutcome_=expeditionRoll(seed_,dest.difficulty,traveler_.level,typeMatch);
 pending_=expeditionRewardPlan(destination_,pendingOutcome_,seed_);
 walletTarget_=pet.wallet+pending_.coins;
 if(walletTarget_<pet.wallet||walletTarget_>Pet::WALLET_CAP)walletTarget_=Pet::WALLET_CAP;
 memset(itemTargets_,0,sizeof(itemTargets_));
 for(uint8_t i=0;i<pending_.itemCount;i++){
   ItemKey k=pending_.items[i].key;
   uint16_t target=uint16_t(bag.count(k))+pending_.items[i].count;
   for(uint8_t j=0;j<i;j++)if(pending_.items[j].key==k)target=itemTargets_[j]+pending_.items[i].count;
   if(target>BAG_STACK_MAX)target=BAG_STACK_MAX;
   itemTargets_[i]=(uint8_t)target;
 }
 ExpeditionJournalEntry e;
 e.destination=destination_;e.outcome=(uint8_t)pendingOutcome_;
 e.coins=(uint16_t)pending_.coins;e.decoration=pending_.decoration;
 for(int i=7;i>0;i--)history_[i]=history_[i-1];
 history_[0]=e;if(journalCount_<8)journalCount_++;
 state_=2;
 if(!persist()){
   state_=1;
   for(int i=0;i<7;i++)history_[i]=history_[i+1];
   history_[7]=ExpeditionJournalEntry();if(journalCount_)journalCount_--;
   return false;
 }
 reward=pending_;outcome=pendingOutcome_;
 return settle();
}
