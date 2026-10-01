module;

#include <stdafx.h>
#include "common.h"

export module MenuConstraint;

import Skeleton;
import Draw;
import Menu;
import Frontend;

namespace MenuConstraintHooks
{
    // Switching screens draws the outgoing menu into the transition buffer.
    constexpr bool InputDrawsMenu = true;
    constexpr const char* TransitionSignature = "53 55 89 CD 8B 5C 24 0C 68 FF 00 00 00 6A 00 6A 00 6A 00 6A 00 6A 00 6A 00 C6 85 10 01 00 00 01";
    bool IsCurrentFrontend() { return true; }
    // The menu frame limiter replaces the first call with a mid-hook jump.
    constexpr const char* DrawSignature = "53 89 CB FF 35 ? ? ? ? ? ? ? ? ? 59 E8 ? ? ? ? E8 ? ? ? ? 6A 00 6A 06";
    constexpr const char* InputSignature = "30 D2 53 56 57 55 89 CD 83 EC 30 88 54 24 2F 8A 44 24 2F";
    constexpr const char* MouseSignature = "C6 44 24 14 00 8B 85 F8 00 00 00 83 F8 15 74 ? 83 F8 1E";
    void* MouseManager(SafetyHookContext& regs) { return reinterpret_cast<void*>(regs.ebp); }
    constexpr const char* VideoSignature = "53 83 EC 10 6A 00 E8 ? ? ? ? 59 B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 6A 00 6A 16";
    constexpr const char* CentreSignature = nullptr;
    constexpr const char* PreviewSignature = "53 56 57 83 EC 28 80 3D ? ? ? ? 00 75 11 C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 00 00 00 00 BE";
    int __cdecl RenderPreview();
    constexpr auto PreviewCallback = RenderPreview;
    constexpr const char* FontSignature = "E8 ? ? ? ? 6A ? E8 ? ? ? ? 59 8D 8C 24 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? ? ? ? ? ? ? 59 50 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 50 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 3D";
    constexpr std::array<size_t, 3> MouseFields{ offsetof(CMenuManager, m_nMousePosX), offsetof(CMenuManager, m_nMouseTempPosX), offsetof(CMenuManager, m_nMouseOldPosX) };
    static_assert(MouseFields == std::array<size_t, 3>{ 0x12C, 0x64, 0x134 });
    uintptr_t FontAddress(uintptr_t address) { return injector::GetBranchDestination(address).as_int(); }
    bool UsesNativeCanvas(void* menu)
    {
        // A replacement map owns both its drawing and pixel-space input.
        return !g_externalMenuMap || static_cast<CMenuManager*>(menu)->m_nCurrScreen != 6;
    }
}

#include <GTA/MenuConstraint.inl>

int __cdecl MenuConstraintHooks::RenderPreview()
{
    MenuCanvas::Suspend physicalViewport;
    return Preview.unsafe_ccall<int>();
}
