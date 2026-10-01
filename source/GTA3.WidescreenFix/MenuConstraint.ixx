module;

#include <stdafx.h>
#include "common.h"

export module MenuConstraint;

import Skeleton;
import Draw;
import Frontend;

namespace MenuConstraintHooks
{
    constexpr bool InputDrawsMenu = false;
    constexpr const char* TransitionSignature = nullptr;
    bool IsCurrentFrontend() { return true; }
    // The menu frame limiter replaces the first call with a mid-hook jump.
    constexpr const char* DrawSignature = "53 89 CB FF 35 ? ? ? ? ? ? ? ? ? 83 BB 48 05 00 00 00 59 75 22";
    constexpr const char* InputSignature = "53 56 57 55 83 EC 70 83 3D ? ? ? ? 00 89 CD 74 0E";
    constexpr const char* MouseSignature = "8B 85 48 05 00 00 83 F8 1E 74 ? 83 F8 36 74 ? 83 F8 37";
    void* MouseManager(SafetyHookContext& regs) { return reinterpret_cast<void*>(regs.ebp); }
    constexpr const char* VideoSignature = "6A 00 E8 ? ? ? ? 59 B9 ? ? ? ? E8 ? ? ? ? 8B 44 24 04 50 E8 ? ? ? ? 50 E8";
    constexpr const char* FontSignature = "A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 40 50 E8 ? ? ? ? A1 ? ? ? ? 59 83 C0 02";
    constexpr const char* CentreSignature = "D9 EE DB 05 ? ? ? ? 83 EC 18 89 4C 24 0C D8 0D ? ? ? ? DD D9 DB 05 ? ? ? ? D8 0D ? ? ? ?";
    constexpr const char* PreviewSignature = "53 56 57 83 EC 38 80 3D ? ? ? ? 00 75 11 C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 00 00 00 00 BE";
    void __cdecl RenderPreview();
    constexpr auto PreviewCallback = RenderPreview;
    constexpr std::array<size_t, 3> MouseFields{ 0x118, 0x120, 0x53C };
    uintptr_t FontAddress(uintptr_t address) { return address; }
    bool UsesNativeCanvas(void*) { return true; }
}

#include <GTA/MenuConstraint.inl>

void __cdecl MenuConstraintHooks::RenderPreview()
{
    // The model uses the physical 3D viewport, but its horizontal placement
    // follows the centered menu canvas. Restore only the renderer dimensions.
    const float x = playerSkinPos->x;
    MenuCanvas::Suspend physicalViewport;
    playerSkinPos->x = x;
    Preview.unsafe_ccall();
}
