module;

#include <stdafx.h>
#include <atomic>
#ifdef _DEBUG
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#endif

export module Hud;

import ComVars;

export
{
#ifdef _DEBUG
std::map<uintptr_t, uint32_t> retXmap;
std::map<uintptr_t, uint32_t> retYmap;
#endif


enum HudCoords
{
    HEALTH_CLUSTER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND,
    HEALTH_CLUSTER_BACKGROUND_COORDS__SET_ME_TO_MOVE_HEALTHSLAUGHTER_HUD,
    HEALTH_BAR_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND,
    HEALTH_BAR_WIDTH_HEIGHT_NEED_IT_SINCE_HEALTH_AND_ARMOR_ARE_IN_THE_SAME_METER,
    SLAUGHTER_METER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND,
    SKULL_ICON_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND,
    USE_MESSAGE,
    MAIN_WEAPON_UPPER_LEFT_CORNER,
    WEAPON_ICON_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX,
    AMMO_COUNTER_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX,
    RESERVE_AMMO_BAR_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX,
    RETICLE_OFFSETS_FROM_CENTER_OF_SCREEN,
    HUMAN_SHIELD,
    THE_INTERROGATE_INTERACTIVE_HUD_ELEMENT,
    THE_INTERROGATE_CONTROL_EXPLAINATION,
    AMMO_PICKUP_MESSAGES,
    HUD_PERSONA_BACKGROUND,
    HUD_PERSONA_HEAD,
    FRONT_DAMAGE_INDICATOR_CENTER,
    LEFT_DAMAGE_INDICATOR_CENTER,
    BACK_DAMAGE_INDICATOR_CENTER,
    RIGHT_DAMAGE_INDICATOR_CENTER,
    HUD_MESSAGE_LOG_TEXT_AREA_WIDTH_HEIGHT_IGNORED,
    HUD_COUNTDOWN_TIMER,
    GENERIC_MESSAGE_BACKGROUND_BITMAP_COORDINATES,
    GENERIC_MESSAGE_TEXT_LOCATION,
    GENERIC_MESSAGE_TEXT_WIDTH_IN_PIXELS_LINE_1,
    POSITION_OF_OBJECTIVES_POPUP_FRAME,
    COMBO_SCORE_DISPLAY,
    LOADING_PROGRESS_BAR_NOTE_NOT_A_HUD_ITEM,
    TUTORIAL_MESSAGE,
    TUTORIAL_MESSAGE_WIDTH_Y_IS_IGNORED,
    CHALLENGE_MODE_TEXT_X_IGNORED_CAUSE_ITS_CENTERED_ON_THE_SCREEN,
    PUNISHMENT_MODE_TEXT_X_IGNORED,
    PUNISHMENT_MODE_COUNTER,
    SECONDARY_HEALTH_BAR_X_IGNORED,
    SUBTITLE_YPOS_X_IS_USED_TO_LIMIT_WIDTH_MESSAGE_WIDTH__SCREEN_WIDTH__2X
};



struct HudCoordinate
{
    int32_t* destination;
    size_t kind;
    int32_t x, y;
};
std::vector<HudCoordinate> HudCoordinates;
std::mutex HudCoordinatesMutex;
std::atomic<float> RequestedHudAspect{ 0.0f };
std::atomic_bool HudCoordinatesPending{ false };
int(__cdecl* SelectHudLayout)(int) = nullptr;

bool StaticHudCoordinate(const int32_t* destination)
{
    static const auto ranges = []()
    {
        std::vector<std::pair<uintptr_t, uintptr_t>> result;
        const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        auto section = IMAGE_FIRST_SECTION(nt);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
        {
            const auto size = std::max(section->Misc.VirtualSize, section->SizeOfRawData);
            if ((section->Characteristics & IMAGE_SCN_MEM_WRITE) && size >= 8)
                result.emplace_back(base + section->VirtualAddress, base + section->VirtualAddress + size);
        }
        return result;
    }();
    const auto address = reinterpret_cast<uintptr_t>(destination);
    return std::any_of(ranges.begin(), ranges.end(), [address](const auto& range)
    {
        return address >= range.first && address <= range.second - 8;
    });
}

std::pair<int32_t, int32_t> FitHudCoordinate(size_t kind, int32_t x, int32_t y)
{
    const float requested = RequestedHudAspect.load(std::memory_order_acquire);
    const float aspect = requested > 0 ? ClampHudAspectRatio(requested, Screen.fAspectRatio) : Screen.fAspectRatio;
    const float width = std::min(Screen.fWidth, Screen.fHeight * aspect);
    const float left = (Screen.fWidth - width) * 0.5f;
    const auto leftAligned = [&]() { return static_cast<int32_t>(std::lround(left + x)); };
    const auto rightAligned = [&]() { return static_cast<int32_t>(std::lround(Screen.fWidth - left - (Screen.PresetWidth - x))); };
    const auto centred = [&]() { return static_cast<int32_t>(std::lround(left + (width - Screen.PresetWidth) * 0.5f + x)); };
    const auto bottom = [&]() { return Screen.Height - (Screen.PresetHeight - y); };
    switch (kind)
    {
    case HEALTH_CLUSTER_BACKGROUND_COORDS__SET_ME_TO_MOVE_HEALTHSLAUGHTER_HUD:
    case HUMAN_SHIELD:
    case HUD_PERSONA_BACKGROUND:
    case HUD_PERSONA_HEAD:
        x = leftAligned();
        y = bottom();
        break;
    case AMMO_PICKUP_MESSAGES:
    case MAIN_WEAPON_UPPER_LEFT_CORNER:
        x = rightAligned();
        y = bottom();
        break;
    case COMBO_SCORE_DISPLAY:
    case HUD_COUNTDOWN_TIMER:
    case THE_INTERROGATE_CONTROL_EXPLAINATION:
        x = rightAligned();
        break;
    case HEALTH_CLUSTER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
    case HEALTH_BAR_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
    case HEALTH_BAR_WIDTH_HEIGHT_NEED_IT_SINCE_HEALTH_AND_ARMOR_ARE_IN_THE_SAME_METER:
    case SLAUGHTER_METER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
    case SKULL_ICON_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
    case WEAPON_ICON_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
    case AMMO_COUNTER_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
    case RESERVE_AMMO_BAR_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
    case RETICLE_OFFSETS_FROM_CENTER_OF_SCREEN:
    case CHALLENGE_MODE_TEXT_X_IGNORED_CAUSE_ITS_CENTERED_ON_THE_SCREEN:
    case PUNISHMENT_MODE_TEXT_X_IGNORED:
    case PUNISHMENT_MODE_COUNTER:
    case SECONDARY_HEALTH_BAR_X_IGNORED:
        break;
    case FRONT_DAMAGE_INDICATOR_CENTER:
        x = Screen.Width / 2 - 176;
        y = 0;
        break;
    case BACK_DAMAGE_INDICATOR_CENTER:
        x = Screen.Width / 2 - 176;
        y = Screen.Height - 70;
        break;
    case LEFT_DAMAGE_INDICATOR_CENTER:
        x = 0;
        y = Screen.Height / 2 - 176;
        break;
    case RIGHT_DAMAGE_INDICATOR_CENTER:
        x = Screen.Width - 64;
        y = Screen.Height / 2 - 176;
        break;
    case LOADING_PROGRESS_BAR_NOTE_NOT_A_HUD_ITEM:
        x = static_cast<int32_t>(x * Screen.PresetFactorX);
        y = static_cast<int32_t>(y * Screen.PresetFactorY);
        break;
    case HUD_MESSAGE_LOG_TEXT_AREA_WIDTH_HEIGHT_IGNORED:
    case GENERIC_MESSAGE_TEXT_WIDTH_IN_PIXELS_LINE_1:
    case TUTORIAL_MESSAGE_WIDTH_Y_IS_IGNORED:
        x = static_cast<int32_t>(x * (width / Screen.PresetWidth));
        break;
    case USE_MESSAGE:
    case GENERIC_MESSAGE_BACKGROUND_BITMAP_COORDINATES:
        x = centred();
        y = bottom();
        break;
    case THE_INTERROGATE_INTERACTIVE_HUD_ELEMENT:
        x = centred();
        y += (Screen.Height - Screen.PresetHeight) / 2;
        break;
    case POSITION_OF_OBJECTIVES_POPUP_FRAME:
        x = leftAligned();
        y += (Screen.Height - Screen.PresetHeight) / 2;
        break;
    case SUBTITLE_YPOS_X_IS_USED_TO_LIMIT_WIDTH_MESSAGE_WIDTH__SCREEN_WIDTH__2X:
        x = leftAligned();
        y = bottom();
        break;
    default:
        x = static_cast<int32_t>(left + x * (width / Screen.PresetWidth));
        break;
    }
    return { x, y };
}

void ApplyPendingHudCoordinates()
{
    if (!HudCoordinatesPending.load(std::memory_order_relaxed)
        || !HudCoordinatesPending.exchange(false, std::memory_order_acquire)) return;
    std::lock_guard lock(HudCoordinatesMutex);
    for (const auto& record : HudCoordinates)
    {
        const auto [x, y] = FitHudCoordinate(record.kind, record.x, record.y);
        record.destination[0] = x;
        record.destination[1] = y;
    }
    if (SelectHudLayout && !HudCoordinates.empty())
        SelectHudLayout(Screen.PresetWidth);
}

int32_t retX()
{
    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("retX addr: 0x{0:08x}", retXmap[((uintptr_t)_ReturnAddress() - 5)]); });

    return Screen.PresetWidth;
};

int32_t retY()
{
    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("retY addr: 0x{0:08x}", retYmap[((uintptr_t)_ReturnAddress() - 5)]); });

    return Screen.PresetHeight;
};



void InitHud()
{
    hook::pattern pattern;
    auto hudSelector = hook::pattern("8B 44 24 04 3D 20 03 00 00 56 57 7F ? 74 ? 3D 00 02 00 00 74 ? 3D 80 02 00 00 75 ? BE ? ? ? ? BF ? ? ? ? B9 4A 00 00 00 F3 A5");
    auto hudFrame = hook::pattern("83 EC 3C 53 8B D8 8B 43 2C 50 E8 ? ? ? ? 8B C8 83 C4 04 85 C9 89 4C 24 1C 0F 84 ? ? ? ? 8B 81 78 08 00 00");
    SelectHudLayout = reinterpret_cast<int(__cdecl*)(int)>(hudSelector.get_first());
    static auto HudLayoutFrame = safetyhook::create_mid(hudFrame.get_first(), [](SafetyHookContext&)
    {
        ApplyPendingHudCoordinates();
    });
    if (!HudLayoutFrame) SelectHudLayout = nullptr;

    //Resolution
    pattern = hook::pattern("89 1D ? ? ? ? A3 ? ? ? ? 88 0D");
    dword_913250.SetAddress(*pattern.get_first<int32_t*>(2));
    dword_913254.SetAddress(*pattern.get_first<int32_t*>(7));
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_913250 = Screen.Width;
            dword_913254 = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(11));

    pattern = hook::pattern("C7 04 24 80 02 00 00"); //0x4DD6CB
    injector::MakeNOP(pattern.get_first(-2), 2, true);
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(3), Screen.Width, true);

    pattern = hook::pattern("C7 44 24 04 E0 01 00 00"); //0x4DD6ED
    injector::MakeNOP(pattern.get_first(-2), 2, true);
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(4), Screen.Height, true);

    // Keep HUD layouts and the native menu preset at their original dimensions.
    auto hudPreset = hook::pattern("E8 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 6A 01");
    auto menuPreset = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 50 8D 44 24 38 68 ? ? ? ? 50");
    injector::MakeCALL(hudPreset.get_first(), retX, true);
    injector::MakeCALL(hudPreset.get_first(10), retY, true);
    injector::MakeCALL(menuPreset.get_first(), retY, true);
    injector::MakeCALL(menuPreset.get_first(6), retX, true);

    //HUD
    pattern = hook::pattern("68 ? ? ? ? 8B CF E8 ? ? ? ? 84 C0 ? ? 56 8B 74 24 08"); //0x4632C0
    static auto str_5F5588 = *pattern.get_first<uint32_t>(1);
    static size_t curLine;
    struct iHudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            curLine = 0;
            *(uint32_t*)regs.esp = str_5F5588; //"#End"
            regs.ecx = regs.edi;
        }
    };
    injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(0), 0x56, true);
    injector::MakeInline<iHudHook>(pattern.get_first(1), pattern.get_first(7));

    pattern = hook::pattern("68 ? ? ? ? 8B CF 83 C6 08 E8"); //0x4632E8
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto x = *(int32_t*)(regs.esi + 0x00);
            auto y = *(int32_t*)(regs.esi + 0x04);

            auto destination = reinterpret_cast<int32_t*>(regs.esi);
            constexpr size_t count = SUBTITLE_YPOS_X_IS_USED_TO_LIMIT_WIDTH_MESSAGE_WIDTH__SCREEN_WIDTH__2X + 1;
            if (curLine < count && StaticHudCoordinate(destination))
            {
                std::lock_guard lock(HudCoordinatesMutex);
                if (!curLine)
                {
                    const auto start = reinterpret_cast<uintptr_t>(destination);
                    const auto end = start + count * 2 * sizeof(int32_t);
                    std::erase_if(HudCoordinates, [start, end](const auto& record)
                    {
                        const auto address = reinterpret_cast<uintptr_t>(record.destination);
                        return address >= start && address < end;
                    });
                }
                HudCoordinates.push_back({ destination, curLine, x, y });
            }
            std::tie(x, y) = FitHudCoordinate(curLine, x, y);

            *reinterpret_cast<int32_t*>(regs.esi) = x;
            *reinterpret_cast<int32_t*>(regs.esi + 4) = y;

            *(uint32_t*)regs.esp = str_5F5588; //"#End"
            regs.ecx = regs.edi;
            ++curLine;
        }
    };
    injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(0), 0x56, true);
    injector::MakeInline<HudHook>(pattern.get_first(1), pattern.get_first(7));


}

}
