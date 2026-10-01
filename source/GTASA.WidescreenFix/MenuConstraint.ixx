module;

#include <stdafx.h>
#include "common.h"

export module MenuConstraint;

import Skeleton;
import Draw;
import Menu;
import Frontend;
import MenuMap;

namespace MenuConstraintHooks
{
    // Process includes tile streaming, input and the dialogs displayed by them.
    constexpr bool InputDrawsMenu = true;
    constexpr const char* TransitionSignature = nullptr;
    bool IsCurrentFrontend()
    {
        CIniReader reader("");
        return reader.ReadInteger("MAIN", "ScalingMode", 1) != 0;
    }
    // The 1.0 retail executable rewrites the following call during DRM setup.
    constexpr const char* DrawSignature = "56 8B F1 8A 46 32 84 C0 0F 85 ? ? ? ? 68 00 00 7F 43";
    constexpr const char* InputSignature = "56 8B F1 8A 46 5C 84 C0 74 2A 33 C0 8A 46 5B 50 E8";
    constexpr const char* MouseSignature = "80 BE 5D 01 00 00 26 C6 44 24 ? 00 75 ? 8A 86 EA 1A 00 00";
    void* MouseManager(SafetyHookContext& regs) { return reinterpret_cast<void*>(regs.esi); }
    constexpr const char* VideoSignature = "8B 44 24 04 50 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? A1 ? ? ? ? 8B 48 60 8B 51 0C";
    constexpr const char* CentreSignature = "83 EC 10 DB 05 ? ? ? ? D8 0D ? ? ? ? DB 05 ? ? ? ? D8 0D ? ? ? ? D9 5C 24 0C D8 1D";
    constexpr const char* FontSignature = "E8 ? ? ? ? 8B CE E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 11 A1 ? ? ? ? A3 ? ? ? ? C6 05";
    constexpr const char* PreviewSignature = nullptr;
    constexpr void (*PreviewCallback)() = nullptr;
    constexpr std::array<size_t, 3> MouseFields{ offsetof(CMenuManager, m_nMousePosX), offsetof(CMenuManager, m_nMousePosWinX), 0x1AF8 };
    static_assert(MouseFields == std::array<size_t, 3>{ 0xBC, 0xE0, 0x1AF8 });
    uintptr_t FontAddress(uintptr_t address) { return injector::GetBranchDestination(address).as_int(); }
    bool UsesNativeCanvas(void* menu)
    {
        auto manager = static_cast<CMenuManager*>(menu);
        return manager->m_bMenuActive && !(bFullscreenMap && manager->GetCurrentScreen() == SCREEN_MAP);
    }
}

#include <GTA/MenuConstraint.inl>
