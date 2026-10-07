#pragma once
#include "Addresses.hpp"
#include "../Shared/Console/PSP.hpp"
namespace ctw {
using namespace console::portable;
inline constexpr float nativeAspect = 480.0f / 272.0f;
inline float targetAspect = nativeAspect;
inline bool automaticAspect = true;
inline float HorizontalScale() { return nativeAspect / targetAspect; }
void RefreshAspect();
template<class T> inline T& at(uintptr_t address) { return *reinterpret_cast<T*>(address); }
inline bool Guest(uintptr_t address, unsigned size = 4) {
    return address >= 0x08000000 && address <= 0x0DD00000 - size;
}
inline uintptr_t Player() {
    const unsigned index = at<unsigned>(Address<0x08B5B238>());
    if (index >= 2) return 0;
    const auto player = at<uintptr_t>(Address<0x08C0D160>() + index * 4);
    return Guest(player, 0x1000) ? player : 0;
}
inline uintptr_t Vehicle() {
    const auto player = Player();
    if (!player) return 0;
    const auto handle = at<uintptr_t>(player + 0x1CC);
    return Guest(handle) ? at<uintptr_t>(handle) : 0;
}
// The PDA's running application is 16 while the gameplay HUD is shown; the
// PDA, apartment, menus and minigame screens use other applications.
inline bool GameplayApp() {
    const auto app = at<uintptr_t>(Address<0x08C11258>());
    return Guest(app, 200) && at<int>(app + 196) == 16;
}
inline bool GameplayCamera() {
    const auto player = Player();
    return player && Vehicle() && !at<uint8_t>(player + 0xE31);
}
void InstallInput();
void InstallCamera();
void InstallAspect();
void InstallReplay();
void InstallHud();
void InstallVideo();
}
