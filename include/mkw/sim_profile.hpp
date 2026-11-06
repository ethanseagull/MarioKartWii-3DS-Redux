#pragma once
// DIAG physics breakdown (Old 3DS work, Nov 6): where one physics tick's time
// goes. Scopes inside Kinoko's RaceScene::calcEngines and the kart step add
// system ticks to fixed slots; main logs per-tick averages ("sim profile")
// every 600 ticks of racing. Off (no clock reads) until data.on is set.
// Off the 3DS (host Kinoko builds) every scope is empty.
#if defined(__3DS__)
extern "C" { // <3ds.h> adds this; these two headers alone do not
#include <3ds/types.h>
#include <3ds/svc.h>
}
#endif
namespace mkw::simprof {
enum Slot : unsigned {
    RaceMgr, BoxCol, Objects, Karts, Jugem, Items,           // RaceScene::calcEngines
    K0State, K0Move, K0Action, K0Physics, K0Hitboxes,        // KartSub::calcPass0
    K1Total, K1Search, K1KartCol, K1ObjCol, K1ItemCol, K1Body, K1Wheels, // KartSub::calcPass1
    K1Model,                                                 // KartObject::calc: model()->calc()
    Kinoko,                                                  // main: scenes.calc()
    Count
};
inline constexpr const char* Names[Count]={
    "race_manager","box_col","objects","karts","jugem","items",
    "k0_state","k0_move","k0_action","k0_physics","k0_hitboxes",
    "k1_total","k1_box_search","k1_kart_col","k1_object_col","k1_item_col","k1_body_col","k1_wheels",
    "k1_model","kinoko"};
struct Data {unsigned long long ticks[Count]{};unsigned steps=0;bool on=false;};
inline Data data{};
#if defined(__3DS__)
struct Scope {
    Slot slot;u64 start;
    explicit Scope(Slot s):slot(s),start(data.on?svcGetSystemTick():0){}
    ~Scope(){if(data.on)data.ticks[slot]+=svcGetSystemTick()-start;}
    Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
};
#else
struct Scope {explicit Scope(Slot){}};
#endif
}
#define MKW_SIM_SCOPE(slot) ::mkw::simprof::Scope mkwSimScope_##slot(::mkw::simprof::slot)
