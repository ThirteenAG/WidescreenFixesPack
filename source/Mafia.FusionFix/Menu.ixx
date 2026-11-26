module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cmath>
#include <unordered_set>

export module Menu;

import ComVars;

uintptr_t menuRawCoordinates;
std::array<uintptr_t, 5> menuCallers;
bool menuHooksInstalled;
uintptr_t* menuPanelTable;
uintptr_t* menuImageTable;

namespace LegacyMenu
{
    uintptr_t* topPage;
    float* scaleX;
    float* scaleY;
    std::unordered_set<uintptr_t> centered;
    std::unordered_set<uintptr_t> cachedPosition;

    void Resize(float scale, float shift, float previousWidth, float previousHeight)
    {
        if (!*topPage)
            return;
        *scaleX *= scale;
        *scaleY *= scale;
        // The page owns a flat array of controls. Container controls also cache a position including their parent.
        for (auto page = *topPage; page; page = Field<uintptr_t>(page, 0x18))
        {
            // Preview controls set an inset scene rectangle when they draw.
            auto& width = Field<float>(page, 0x10);
            auto& height = Field<float>(page, 0x14);
            if (Field<float>(page, 0x08) == 0.0f && Field<float>(page, 0x0C) == 0.0f &&
                width == previousWidth && height == previousHeight)
            {
                width = Screen.fWidth;
                height = Screen.fHeight;
            }
            auto begin = Field<uintptr_t*>(page, 0x20);
            auto end = Field<uintptr_t*>(page, 0x24);
            for (auto item = begin; item != end; ++item)
            {
                auto node = *item;
                if (!node) continue;
                Field<float>(node, 0x10) = Field<float>(node, 0x10) * scale + (centered.contains(node) ? shift : 0.0f);
                for (size_t offset : { 0x14, 0x18, 0x1C })
                    Field<float>(node, offset) *= scale;
                if (cachedPosition.contains(node))
                {
                    auto& x = Field<float>(node, 0x38);
                    x *= scale;
                    for (auto parent = node; parent; parent = Field<uintptr_t>(parent, 0x34))
                        if (centered.contains(parent)) x += shift;
                    Field<float>(node, 0x3C) *= scale;
                }
            }
        }
    }
}

// Private native pages use their own templates, so their root has no original template address.
export void CenterLegacyMenuRoot(uintptr_t root)
{
    if (!LegacyMenu::centered.insert(root).second) return;
    Field<float>(root, 0x10) += fMenuOffset;
    if (LegacyMenu::cachedPosition.contains(root))
        Field<float>(root, 0x38) += fMenuOffset;
}

export uintptr_t* LegacyMenuStack()
{
    static auto* stack = *hook::pattern("56 8B F1 8B 46 18 A3 ? ? ? ? 8B 4E 18 85 C9 74 ? 8B 44 24 0C 8B 11").get_first<uintptr_t*>(7);
    return stack;
}
bool CenterMenuControl(bool panel, bool image, uint32_t id, uint32_t flags)
{
    return (panel && (id == 1 || id == 3 || id == 0x82 ||
        (id == 2 && (flags == 0xFF00B2 || flags == 0xFF0032 || flags == 0x1F0032)))) ||
        (image && id >= 1 && id <= 3);
}

void InstallMenuHooks()
{
    auto pMenuImagesOld = find_pattern("E8 ? ? ? ? D9 45 10 8B 45 34 C7 45 00 ? ? ? ? D9 55 38 D9 45 14 3B C3 D9 54 24 20 D9 5D 3C 74 ? D8 40 38 D9 5D 38 D9 44 24 20 D8 40 3C C7 45 00 ? ? ? ? 8B C5 D9 5D 3C E9 ? ? ? ? DD D8 C7 45 00 ? ? ? ? 8B C5 E9 ? ? ? ? 81 E9 64 6E 69 77");
    auto pMenuPanelsOld = find_pattern("E8 ? ? ? ? D9 45 10 8B 45 34 C7 45 00 ? ? ? ? D9 55 38 D9 45 14 3B C3 D9 54 24 20 D9 5D 3C 74 ? D8 40 38 D9 5D 38 D9 44 24 20 D8 40 3C D9 5D 3C EB ? DD D8 C7 45 00 ? ? ? ? 8B C5");
    auto pMenuGarageOld = find_pattern("E8 ? ? ? ? D9 45 10 8B 45 34 C7 45 00 ? ? ? ? D9 55 38 D9 45 14 3B C3 D9 54 24 20 D9 5D 3C 74 ? D8 40 38 D9 5D 38 D9 44 24 20 D8 40 3C 8B C5");
    auto pMenuRaceOld = find_pattern("E8 ? ? ? ? D9 46 10 8B 46 34 C7 06 ? ? ? ?");
    auto pMenuCarsOld = find_pattern("E8 ? ? ? ? 8B 46 08 C7 06 ? ? ? ? 0D 01 00 01 00");
    auto pMenuPanelsNew = find_pattern("E8 ? ? ? ? C7 06 ? ? ? ? 8B C6 8B 4C 24 10 5F 5E 5B 64 89 0D 00 00 00 00 83 C4 10 C3 90 90 90 90 90 90 B8 64 6E 69 77");
    auto pMenuImagesNew = find_pattern("E8 ? ? ? ? C7 06 ? ? ? ? 8B C6 8B 4C 24 10 5F 5E 5B 64 89 0D 00 00 00 00 83 C4 10 C3 90 90 90 90 90 90 B8 63 6E 69 77");
    if (menuHooksInstalled) return;
    menuHooksInstalled = true;
    if (!pMenuImagesOld.empty()) // 1.0 menu layout
    {
        menuCallers = {reinterpret_cast<uintptr_t>(pMenuImagesOld.get_first()) + 5, reinterpret_cast<uintptr_t>(pMenuPanelsOld.get_first()) + 5,
            reinterpret_cast<uintptr_t>(pMenuGarageOld.get_first()) + 5, reinterpret_cast<uintptr_t>(pMenuRaceOld.get_first()) + 5, reinterpret_cast<uintptr_t>(pMenuCarsOld.get_first()) + 5};
        auto pCoordinateMode = find_pattern("8A 15 ? ? ? ? 3A D1 74 ? 8B 50 08 89 56 10");
        LegacyMenu::topPage = LegacyMenuStack();
        auto scales = hook::pattern("D9 1D ? ? ? ? 8B 10 FF 52 70 89 44 24 18 33 FF DB 44 24 18 D8 0D ? ? ? ? D9 1D");
        LegacyMenu::scaleX = *scales.get_first<float*>(2);
        LegacyMenu::scaleY = *scales.get_first<float*>(29);
        auto destroy = hook::pattern("8B 0E 85 C9 74 06 8B 01 6A 01 FF 10 8B 47 24 83 C6 04 3B F0");
        static auto destroyHook = safetyhook::create_mid(destroy.get_first(6), [](SafetyHookContext& regs)
        {
            LegacyMenu::centered.erase(regs.ecx);
            LegacyMenu::cachedPosition.erase(regs.ecx);
        });
        auto flag = reinterpret_cast<uintptr_t>(pCoordinateMode.get_first());
        auto pPosition = find_pattern("8B 50 1C 89 4C 24 08 89 56 20 8B 50 20");
        auto position = reinterpret_cast<uintptr_t>(pPosition.get_first());
        menuRawCoordinates = Field<uintptr_t>(flag, 2);
        static auto MenuPositionHook = safetyhook::create_mid(position, [](SafetyHookContext& regs)
        {
            auto caller = Field<uintptr_t>(regs.esp, 0xC);
            if (caller == menuCallers[0] || caller == menuCallers[1] || caller == menuCallers[2])
                LegacyMenu::cachedPosition.insert(regs.esi);
            if (Field<uint8_t>(menuRawCoordinates, 0)) return;
            auto templateOffset = regs.eax - menuRawCoordinates;
            // Offsets within the original menu-template buffer, whose base is
            // discovered from the constructor's native coordinate-mode flag.
            static constexpr std::array<uint32_t, 23> panels = {
                0x2B4, 0x200, 0x638, 0x2474, 0x2840, 0x695C, 0x4148, 0x4484, 0x45C8,
                0xDF4, 0xECC, 0xF14, 0xF5C, 0x8E4, 0x6CBC, 0x80C, 0x2A80, 0x2AC8,
                0x4E80, 0x4F58, 0x2E4C, 0x65D8, 0x6470};
            bool shift = caller == menuCallers[1] && std::ranges::find(panels, templateOffset) != panels.end();
            shift |= caller == menuCallers[0] && (templateOffset == 0x6788 || templateOffset == 0x266C);
            shift |= (caller == menuCallers[2] || caller == menuCallers[3] || caller == menuCallers[4]) &&
                (templateOffset == 0x5030 || templateOffset == 0x4970 || templateOffset == 0x278C);
            if (shift)
            {
                LegacyMenu::centered.insert(regs.esi);
                Field<float>(regs.esi, 0x10) += fMenuOffset;
            }
        });
    }
    else // 1.1/1.2 menu layout
    {
        menuCallers[0] = reinterpret_cast<uintptr_t>(pMenuPanelsNew.get_first()) + 5;
        menuCallers[1] = reinterpret_cast<uintptr_t>(pMenuImagesNew.get_first()) + 5;
        menuPanelTable = *pMenuPanelsNew.get_first<uintptr_t*>(7);
        menuImageTable = *pMenuImagesNew.get_first<uintptr_t*>(7);
        // Both X copies and Y are stored here; the x87 stack is empty. Shift
        // before the native constructor attaches the object to its parent.
        auto pPosition = find_pattern("D9 5E 18 D9 05 ? ? ? ? D8 48 10 D9 5E 1C D9 05 ? ? ? ? D8 48 14");
        auto position = reinterpret_cast<uintptr_t>(pPosition.get_first());
        static auto MenuPositionHook = safetyhook::create_mid(position + 3, [](SafetyHookContext& regs)
        {
            auto caller = Field<uintptr_t>(regs.esp, 0x18);
            auto type = Field<uint32_t>(regs.eax, 0);
            auto flags = Field<uint32_t>(regs.eax, 0x1C);
            bool shift = CenterMenuControl(caller == menuCallers[0], caller == menuCallers[1], type, flags);
            if (shift)
            {
                Field<float>(regs.esi, 0x14) += fMenuOffset;
                Field<float>(regs.esi, 0x24) += fMenuOffset;
            }
        });
    }
}

namespace
{
    float menuPleaseWaitTextX;
}

export void RefreshMenuGeometry()
{
    static float previousWidth, previousHeight;
    const float previousOffset = fMenuOffset;
    menuPleaseWaitTextX = 780.0f + (Screen.fBaseWidth - 800.0f) * 0.5f;
    fMenuOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) * 0.5f;
    const float oldWidth = std::exchange(previousWidth, Screen.fWidth);
    const float oldHeight = std::exchange(previousHeight, Screen.fHeight);
    if (!oldHeight)
        return;

    const float scale = Screen.fHeight / oldHeight;
    const float shift = fMenuOffset - previousOffset * scale;
    if (LegacyMenu::topPage)
    {
        LegacyMenu::Resize(scale, shift, oldWidth, oldHeight);
        return;
    }
    if (!pTopMenuPage.is_initialized() || !pTopMenuPage.get())
        return;
    fMenuScaleX = fMenuScaleX.get() * scale;
    fMenuScaleY = fMenuScaleY.get() * scale;
    for (auto page = pTopMenuPage.get(); page; page = page->parent)
    {
        for (auto item = page->begin; item != page->end; ++item)
        {
            auto control = *item;
            if (!control) continue;
            float absoluteShift = 0.0f, relativeShift = 0.0f;
            for (auto ancestor = control; ancestor; ancestor = ancestor->parent)
            {
                if (CenterMenuControl(ancestor->table == menuPanelTable, ancestor->table == menuImageTable,
                    ancestor->id, ancestor->flags & 0x7FFFFFFF))
                {
                    absoluteShift += shift;
                    if (ancestor == control)
                        relativeShift = shift;
                }
            }
            control->x = control->x * scale + absoluteShift;
            control->relativeX = control->relativeX * scale + relativeShift;
            control->y *= scale;
            control->relativeY *= scale;
            control->width *= scale;
            control->height *= scale;
        }
    }
}

export void InitMenu()
{
    auto pOnePCurBaseWidth4 = find_pattern("D8 0D ? ? ? ? D9 1D ? ? ? ? 8B 10 FF 52 70",
        "D8 0D ? ? ? ? EB ? 8B 10 FF 52 70 89 44 24 18");
    auto pOnePCurBaseWidth3 = find_pattern("D8 0D ? ? ? ? EB ? 8B 10 FF 52 70 89 44 24 1C");
    auto pHudPleaseWaitTextX = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? A1 ? ? ? ? 50 8B 10",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? A1 ? ? ? ? 50 8B 08");
    auto pHudPleaseWaitTextX2 = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? A1 ? ? ? ? 50 8B 08",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? A1 ? ? ? ? 50 8B 10");

    if (!pOnePCurBaseWidth3.empty()) // 1.1/1.2 menu layout
        injector::WriteMemory(pOnePCurBaseWidth3.get_first(0x2), &Screen.fInvBaseWidth, true);
    else // 1.0 menu layout
    {
        auto legacyWidth = hook::pattern("D8 0D ? ? ? ? D9 1D ? ? ? ? 8B 10 FF 52 70");
        legacyWidth.get_first();
    }
    injector::WriteMemory(pOnePCurBaseWidth4.get_first(0x2), &Screen.fInvBaseWidth, true);

    RefreshMenuGeometry();
    injector::WriteMemory(pHudPleaseWaitTextX.get_first(0x2), &menuPleaseWaitTextX, true);
    injector::WriteMemory(pHudPleaseWaitTextX2.get_first(0x2), &menuPleaseWaitTextX, true);
    InstallMenuHooks();
}
