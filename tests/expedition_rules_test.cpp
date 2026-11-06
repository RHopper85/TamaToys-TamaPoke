#include "../expedition_rules.h"
#include <cassert>
#include <cstdio>
int main(){
  assert(EXP_DEST_COUNT==7);
  assert(!expeditionEligible(1,true,false));
  assert(!expeditionEligible(5,false,false));
  assert(!expeditionEligible(3,true,true));
  assert(expeditionEligible(2,true,false));
  assert(!expeditionFinished(100,999,15));
  assert(expeditionFinished(100,1000,15));
  assert(!expeditionFinished(100,50,15));
  for (int d=1;d<=4;d++) for(uint32_t seed=0;seed<1000;seed++){
    auto o=expeditionRoll(seed,d,30);assert(o>=EXP_SUCCESS&&o<=EXP_FAILED);
    assert(expeditionRoll(seed,d,30)==o);
    assert(expeditionCoins(d,EXP_FAILED,seed)==0);
    assert(!expeditionRareDrop(EXP_FAILED,d,seed));
  }
  for (uint8_t d=0;d<EXP_DEST_COUNT;d++) {
    assert(expeditionDecoration(d,EXP_FAILED,1234)==0);
    for (uint32_t seed=0;seed<256;seed++) {
      uint8_t result=expeditionDecoration(d,EXP_SUCCESS,seed);
      assert(result==0 || result==EXP_DESTS[d].commonDecor || result==EXP_DESTS[d].rareDecor);
    }
  }
  std::puts("expedition_rules_test: PASS");
}
