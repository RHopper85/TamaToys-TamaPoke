#include "../expedition_rules.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main(){
 assert(EXP_DEST_COUNT==7);
 assert(!expeditionEligible(1,true,false));
 assert(!expeditionEligible(2,false,false));
 assert(!expeditionEligible(2,true,true));
 assert(expeditionEligible(2,true,false));
 assert(!expeditionFinished(100,999,15));
 assert(expeditionFinished(100,1000,15));
 assert(!expeditionFinished(100,50,15));
 for(int d=1;d<=4;d++)for(uint32_t seed=0;seed<1000;seed++){
   auto outcome=expeditionRoll(seed,d,30);
   assert(outcome>=EXP_SUCCESS&&outcome<=EXP_FAILED);
   assert(expeditionRoll(seed,d,30)==outcome);
   assert(expeditionCoins(d,EXP_FAILED,seed)==0);
 }
 assert(expeditionSuccessChance(3,50,true)==expeditionSuccessChance(3,50,false)+10);
 for(uint8_t d=0;d<EXP_DEST_COUNT;d++){
   const auto &dest=EXP_DESTS[d];
   assert(dest.commonDecor && dest.minutes>0);
   for(uint8_t o=0;o<3;o++){
     ExpeditionOutcome outcome=(ExpeditionOutcome)(o+1);
     uint32_t hits=0;
     for(uint32_t seed=0;seed<100000;seed++){
       uint8_t item=expeditionDecoration(d,outcome,seed);
       if(item){
         hits++;
         assert(item==dest.commonDecor || item==dest.rareDecor);
       }
       auto r=expeditionRewardPlan(d,outcome,seed);
       assert(r.decoration==item);
       if(outcome==EXP_FAILED){
         assert(r.coins==0 && r.itemCount==0 && r.decoration==0);
       }else if(outcome==EXP_PARTIAL){
         assert(r.itemCount<=1);
       }else{
         assert(r.itemCount>=1 && r.itemCount<=3);
       }
       for(int i=0;i<r.itemCount;i++){
         assert(itemValid(r.items[i].key));
         assert(r.items[i].count>=1 && r.items[i].count<=3);
       }
     }
     const double pct=(hits*100.0)/100000.0;
     if(outcome==EXP_SUCCESS){assert(pct>4.5 && pct<5.5);}
     if(outcome==EXP_PARTIAL){assert(pct>0.7 && pct<1.3);}
     std::printf("%s %-15s decor=%.2f%%\n",outcome==EXP_SUCCESS?"SUCCESS":outcome==EXP_PARTIAL?"PARTIAL":"FAILED",dest.name,pct);
   }
 }
 for(int i=0;i<EXP_DEST_COUNT;i++){
   for(int j=i+1;j<EXP_DEST_COUNT;j++){
     assert(EXP_DESTS[i].commonDecor!=EXP_DESTS[j].commonDecor);
     if(EXP_DESTS[i].rareDecor)assert(EXP_DESTS[i].rareDecor!=EXP_DESTS[j].commonDecor);
     if(EXP_DESTS[j].rareDecor)assert(EXP_DESTS[j].rareDecor!=EXP_DESTS[i].commonDecor);
     if(EXP_DESTS[i].rareDecor && EXP_DESTS[j].rareDecor)
       assert(EXP_DESTS[i].rareDecor!=EXP_DESTS[j].rareDecor);
   }
 }
 std::puts("expedition_rules_test: PASS");
}
