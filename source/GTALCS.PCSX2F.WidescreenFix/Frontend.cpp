#include "Game.hpp"
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
}

namespace lcs {
namespace {
safetymips::GameInline<uint64_t()> renderMenus;
safetymips::GameInline<uint64_t(uintptr_t)> processMenus;
pcsx2::GameCallback<int()> mapZoomCallback;
safetymips::GameInline<uint64_t(int64_t, int64_t, int64_t, int64_t)> drawLoading;

// Frontend artwork, fonts and map geometry use the game's 4:3 layout. Keep
// that layout together, then fit it into the current display. In particular,
// the native map changes its vertical scale when this preference is enabled.
struct PreferenceScope {
    uint8_t& preference = *reinterpret_cast<uint8_t*>(address::wideScreenPreference);
    uint8_t saved = preference;
    PreferenceScope() { preference = 0; }
    ~PreferenceScope() { preference = saved; }
};
struct FrontendScope {
    DrawScope drawing{DrawMode::Frontend};
    PreferenceScope preference;
};
int DefaultMapZoom() {
    // 0x3464F8 returns the map page's default zoom: 150 with the widescreen
    // preference, 200 without. The map opens (0x338130, a page handler outside
    // RenderMenus) and resets (CMenuManager::Process 0x33F3A8) with it. The
    // frontend is drawn with the 4:3 layout, so always use the 4:3 zoom.
    return 200;
}
uint64_t RenderMenus() {
    // RenderMenus (0x1F65C8) -> DrawFrontEnd (0x339200): titles, page tabs,
    // button prompts, text pages and the map page (0x33D888) share one layout.
    FrontendScope scope;
    return renderMenus.call();
}
uint64_t ProcessMenus(uintptr_t menu) {
    // CMenuManager::Process (0x33F3A8) handles the map cursor, scrolling and
    // bounds with the same preference byte. Match the 4:3 layout it is drawn in
    // (without it the map opens with the cursor off the player blip).
    PreferenceScope preference;
    return processMenus.call(menu);
}
uint64_t DrawLoading(int64_t progress, int64_t unused, int64_t name, int64_t island) {
    // Only the blocking load loop draws this screen; CPad::Update (Controls.cpp)
    // clears the flag again on the next game frame.
    FrameLimitUnthrottle = settings.unthrottle;
    FrontendScope scope;
    return drawLoading.call(progress, unused, name, island);
}
}
void InstallFrontend() {
    renderMenus = safetymips::create_inline_game(0x1F65C8, RenderMenus);
    processMenus = safetymips::create_inline_game(0x33F3A8, ProcessMenus);
    mapZoomCallback.bind(DefaultMapZoom);
    injector::MakeJMP(0x3464F8, mapZoomCallback.address());
    drawLoading = safetymips::create_inline_game(0x1F5270, DrawLoading);
}
}
