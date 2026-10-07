#include "Game.hpp"
#include "ModernControls.hpp"
namespace ctw {
namespace {
SafetyMipsInline held, pressed, released, repeating, resetRepeat;
// Gameplay functions that also query shared actions 39-51 (L/R aliases used by
// throwing and camera). The frontend dispatcher and PDA keep native bindings.
struct Range { uintptr_t begin, end; };
Range gameplay[22];
void BindGameplay() {
    // Slot zero deliberately stays empty: 0x0888BCAC is the frontend's
    // dispatcher, including PDA confirmations during active gameplay.
    gameplay[1] = {Address<0x088D8C70>(), Address<0x088D90E0>()};
    gameplay[2] = {Address<0x0896E6F4>(), Address<0x0896EA44>()};
    gameplay[3] = {Address<0x089C052C>(), Address<0x089C0E18>()};
    gameplay[4] = {Address<0x089C650C>(), Address<0x089C66B0>()};
    gameplay[5] = {Address<0x089C8200>(), Address<0x089C8274>()};
    gameplay[6] = {Address<0x089CAE2C>(), Address<0x089CB77C>()};
    gameplay[7] = {Address<0x089CC124>(), Address<0x089CCA70>()};
    gameplay[8] = {Address<0x089CD7EC>(), Address<0x089CDB38>()};
    gameplay[9] = {Address<0x089CDDF8>(), Address<0x089CE2F0>()};
    gameplay[10] = {Address<0x089CE2F0>(), Address<0x089CE8B8>()};
    gameplay[11] = {Address<0x089CE8B8>(), Address<0x089CEC10>()};
    gameplay[12] = {Address<0x089CF94C>(), Address<0x089CFA4C>()};
    gameplay[13] = {Address<0x089CFCE8>(), Address<0x089CFE54>()};
    gameplay[14] = {Address<0x089CFE54>(), Address<0x089D0340>()};
    gameplay[15] = {Address<0x089D0340>(), Address<0x089D0D28>()};
    gameplay[16] = {Address<0x089D0D28>(), Address<0x089D10E4>()};
    gameplay[17] = {Address<0x089FC710>(), Address<0x089FC98C>()};
    gameplay[18] = {Address<0x08A2ACAC>(), Address<0x08A2ACE8>()};
    gameplay[19] = {Address<0x08A2ACE8>(), Address<0x08A2AD24>()};
    gameplay[20] = {Address<0x08A6DD1C>(), Address<0x08A6DE68>()};
    gameplay[21] = {Address<0x08AC79A4>(), Address<0x08AC7C50>()};
}
// The gameplay HUD update charges the throw power from held/released action 48
// (native Circle) while the player aims a throwable (reference 08BB54A4/B4).
// That code is loaded with the HUD application after the plugin starts, so the
// two query sites are recognized by their own instructions instead of a range:
//   lbu s1, 0x130(a0)  (player throw-aim flag), later jal query; li a1, 0x30.
bool ThrowPower(uintptr_t caller, int action) {
    if (action != 48 || !Guest(caller - 0x34, 0x34) || at<uint32_t>(caller - 4) != 0x34050030u) return false;
    return at<uint32_t>(caller - 0x24) == 0x90910130u || at<uint32_t>(caller - 0x34) == 0x90910130u;
}

bool Gameplay(uintptr_t caller) {
    for (const auto& range : gameplay) {
        if (caller >= range.begin && caller < range.end) return true;
    }
    return false;
}
bool GameplayPad(uintptr_t pad) {
    const auto player = Player();
    if (!player) return false;
    // cPlayer::GetPad resolves the player's controller slot, then its native
    // action table at +128. Do not remap other players' pads.
    const auto index = at<unsigned>(player + 0xEBC);
    return index < 4 && pad == Address<0x08D57280>() + index * 320 + 128;
}
// Actions 0-38 are on-foot/vehicle gameplay bindings (move, fire, target,
// throw, camera, drive). 39-51 are shared frontend actions (PDA navigation,
// confirm/back, shortcuts) that only remap inside known gameplay functions.
// Remapping by action keeps every query of the same binding consistent:
// e.g. throwables are aimed with held(50)/held(38) and fired by released/
// pressed queries from different functions, which broke the old per-call swap.
bool Remap(uintptr_t pad, int action, uintptr_t caller) {
    if (!Guest(pad, 180) || unsigned(action) >= 52 || !GameplayApp() || !GameplayPad(pad)) return false;
    return action <= 38 || Gameplay(caller) || ThrowPower(caller, action);
}
bool querying;
template<class Invoke> int Query(uintptr_t pad, int action, uintptr_t caller, Invoke invoke) {
    // Repeat/reset queries call the held query internally; map only once.
    if (querying || !Remap(pad, action, caller)) return invoke();
    auto* masks = reinterpret_cast<uint16_t*>(pad + 4);
    const auto original = masks[action];
    masks[action] = ModernMask(original, Vehicle() != 0, masks[4], masks[10]);
    querying = true;
    const int result = invoke();
    querying = false;
    masks[action] = original;
    return result;
}
int Held(uintptr_t pad, int action) {
    return Query(pad, action, uintptr_t(__builtin_return_address(0)), [&] { return held.call<int>(pad, action); });
}
int Pressed(uintptr_t pad, int action) {
    return Query(pad, action, uintptr_t(__builtin_return_address(0)), [&] { return pressed.call<int>(pad, action); });
}
int Released(uintptr_t pad, int action) {
    return Query(pad, action, uintptr_t(__builtin_return_address(0)), [&] { return released.call<int>(pad, action); });
}
int Repeating(uintptr_t pad, int action) {
    return Query(pad, action, uintptr_t(__builtin_return_address(0)), [&] { return repeating.call<int>(pad, action); });
}
int ResetRepeat(uintptr_t pad, int action, uint8_t reset) {
    return Query(pad, action, uintptr_t(__builtin_return_address(0)), [&] { return resetRepeat.call<int>(pad, action, reset); });
}
}
void InstallInput() {
    if (!inireader.ReadInteger("MAIN", "ModernControls", 1)) return;
    BindGameplay();
    safetymips::Options caller;caller.preserve_game_callees=false;
    held = safetymips::create_inline(Address<0x08891160>(), Held, caller);
    pressed = safetymips::create_inline(Address<0x08890E68>(), Pressed, caller);
    released = safetymips::create_inline(Address<0x08890FE4>(), Released, caller);
    repeating = safetymips::create_inline(Address<0x08891414>(), Repeating, caller);
    resetRepeat = safetymips::create_inline(Address<0x088915D4>(), ResetRepeat, caller);
}
}
