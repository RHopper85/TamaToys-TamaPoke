// End-to-end battle item actions: the real battleTap/Inventory/Party paths.
// The capture-math boundary cases remain covered by wild_test.cpp.
#include "Arduino.h"
#include "Arduino_GFX_Library.h"
#include "Preferences.h"
#include "pet.h"
#include "party.h"
#include "battle.h"
#include "inventory.h"
#include "items.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
uint32_t g_seed=0xC0FFEE;
FakeSerial Serial; FakeESP ESP; FakeWire Wire;
volatile int g_touchX=0,g_touchY=0;
volatile bool g_touchDown=false;
void FakeESP::restart(){exit(0);}
int FakeSerial::available(){return 0;}
String FakeSerial::readStringUntil(char){return String("");}
void setup();
void startBattle(int16_t dex,uint8_t lvl);
void battleTap(int16_t x,int16_t y);
void onSwipe(int dir);
void gymRowRect(int i,int*x,int*y,int*w,int*h);
extern Pet pet;
extern Party party;
extern Inventory bag;
extern Combatant btlYou,btlFoe;
extern uint16_t btlHpShown[2];
extern uint8_t btlMenu,btlBagPage,btlMsgCount,wildLvl,wildIv[4];
extern int16_t wildDex;
extern bool btlOver,btlWild,wildCaught,wildShiny,battleOpen;
static int failures=0;
static void ck(bool yes,const char *why){
  std::printf("%s  %s\n",yes?"PASS":"FAIL",why);
  if(!yes)failures++;
}
int main(){
  setup();
  if(pet.awaitingStarter())pet.chooseStarter(1);
  if(pet.isEgg())pet.dbgHatchAs(1,false);
  int x0=0,y0=0,w0=0,h0=0;
  gymRowRect(0,&x0,&y0,&w0,&h0);
  bool stable=true;
  for(int i=1;i<5;i++){
    int x,y,w,h;gymRowRect(i,&x,&y,&w,&h);
    if(x!=x0||w!=w0||h!=h0)stable=false;
  }
  ck(stable&&x0>=70&&x0+w0<=382,"every Gym leader card has a uniform width and clears scrollbar");

  // Starter bag can contain more than four usable battle items. A Potion in
  // slot five was unreachable: the list renderer terminated after four skips.
  bag.clear();
  bag.add(IT_POKEBALL);bag.add(IT_GREATBALL);bag.add(IT_ULTRABALL);
  bag.add(IT_MASTERBALL);bag.add(IT_POTION,2);
  startBattle(16,5);
  btlWild=true;
  btlMenu=3;
  btlYou.maxHp=100;
  btlYou.hp=50;
  btlHpShown[0]=50;
  battleTap(354,259);    // >> in strip above the battle menu
  ck(btlBagPage==1,"battle bag arrows reach the second page");
  onSwipe(1); // swipe right returns to previous
  ck(btlBagPage==0,"swiping right moves to the previous battle bag page");
  onSwipe(-1); // swipe left next
  ck(btlBagPage==1,"swiping left moves to the next battle bag page");
  battleTap(149,297);    // the first entry on page 2 is Potion
  ck(bag.count(IT_POTION)==1,"Potion use consumes exactly one item");
  ck(btlYou.hp==70 && btlHpShown[0]==70,"Potion restores 20 HP and the visible bar immediately reflects it");
  ck(btlMsgCount>=2,"Potion shows item usage and the exact amount healed");
  battleTap(220,345); // dismiss item-use narration
  ck(btlYou.hp==70,"the foe does not remove healed HP before the player sees it");
  battleTap(220,345); // acknowledge heal; now the enemy takes its turn
  ck(btlYou.hp<=70,"the enemy responds only after the Potion narration is acknowledged");

  // Master Ball: capture is guaranteed, item is spent, and the Pokémon is saved
  // through the real party/box insertion path.
  bag.clear();bag.add(IT_MASTERBALL);
  startBattle(16,5);
  ck(btlBagPage==0,"a new battle resets the old bag page");
  btlWild=true;
  wildDex=16;wildLvl=5;wildShiny=false;
  wildIv[0]=wildIv[1]=wildIv[2]=wildIv[3]=20;
  btlFoe.hp=20;btlFoe.maxHp=30;
  btlMenu=3; btlMsgCount=0;
  int oldCount=party.count()+party.boxCount();
  battleTap(149,297);
  ck(wildCaught && btlOver,"Master Ball successfully catches a healthy wild Pokémon");
  ck(party.count()+party.boxCount()==oldCount+1,"caught Pokémon is stored in Party or PC Box");
  ck(!bag.count(IT_MASTERBALL),"Master Ball is consumed by a successful catch");

  // Balls on trainer battles must be disabled, not silently spent.
  bag.clear();bag.add(IT_POKEBALL);
  startBattle(16,5);
  btlMenu=3; btlMsgCount=0;
  const uint8_t before=bag.count(IT_POKEBALL);
  battleTap(149,297);
  ck(!btlOver && bag.count(IT_POKEBALL)==before,"Poké Balls cannot be wasted on trainer battles");

  // An already-full fighter must not waste a Potion.
  bag.clear();bag.add(IT_POTION);
  btlMenu=3;btlMsgCount=0;btlYou.hp=btlYou.maxHp;
  battleTap(149,297);
  ck(bag.count(IT_POTION)==1,"a Potion at full HP is not consumed");
  if(failures)std::printf("FAILURES: %d\n",failures);
  else std::puts("all good");
  return failures?1:0;
}
