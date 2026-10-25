#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include "../Shared/D3D8Compatibility.hpp"
#include "NativeMenu.hpp"
#include "Window.hpp"
#include <atomic>

#ifdef _DEBUG
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
std::map<uintptr_t, uint32_t> retXmap;
std::map<uintptr_t, uint32_t> retYmap;
#endif

char* memstr(char* block, const char* pattern, size_t bsize)
{
    const size_t length = strlen(pattern);
    if (length > bsize)
        return nullptr;
    auto result = std::search(block, block + bsize, pattern, pattern + length);
    return result == block + bsize ? nullptr : result;
}

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

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fDynamicScreenFieldOfViewScale;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
    float fMenuOffsetRealX;
    float fMenuOffsetRealY;
    int32_t PresetWidth;
    int32_t PresetHeight;
    float PresetFactorX;
    float PresetFactorY;
} Screen;

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

std::atomic<int> PunisherFrameLimit{ 60 };

void UpdateFrameLimit()
{
    int limit = FixSettings().Int("MAIN.FPSLimit");
    if (limit == -1)
    {
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        limit = EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1
            ? static_cast<int>(mode.dmDisplayFrequency) : 60;
    }
    PunisherFrameLimit.store(limit > 0 ? std::clamp(limit, 30, 1000) : 0, std::memory_order_relaxed);
}

void Init()
{
    static injector::hook_back<void(__cdecl*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int8_t, int8_t, int32_t, int8_t, int8_t)> hb_519CA0;
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    UpdateFrameLimit();
    settings.OnChange("MAIN.FPSLimit", UpdateFrameLimit);
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    bool bSkipIntro = settings.Bool("MAIN.SkipIntro");
    static const float& fFOVFactor = settings.Float("MAIN.FOVFactor");
    auto updateHudConstraint = []()
    {
        const auto parsed = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        const float aspect = parsed && std::isfinite(*parsed) && *parsed > 0
            ? std::clamp(*parsed, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
        RequestedHudAspect.store(aspect, std::memory_order_release);
        HudCoordinatesPending.store(true, std::memory_order_release);
    };
    updateHudConstraint();
    settings.OnChange("MAIN.HudAspectRatioConstraint", updateHudConstraint);

    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
    {
        Screen.Width = 640;
        Screen.Height = 480;
    }

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //Presets
    static constexpr int32_t heights[] = { 480, 600, 768, 960 };
    Screen.PresetHeight = heights[0];
    for (auto height : heights)
    {
        if (height <= Screen.Height)
            Screen.PresetHeight = height;
    }

    switch (Screen.PresetHeight)
    {
    case 480:
        Screen.PresetWidth = 640;
        break;
    case 600:
        Screen.PresetWidth = 800;
        break;
    case 768:
        Screen.PresetWidth = 1024;
        break;
    case 960:
        Screen.PresetWidth = 1280;
        break;
    default:
        Screen.PresetWidth = 1280;
        Screen.PresetHeight = 960;
        break;
    }

    Screen.PresetFactorX = Screen.fWidth / static_cast<float>(Screen.PresetWidth);
    Screen.PresetFactorY = Screen.fHeight / static_cast<float>(Screen.PresetHeight);

    auto hudSelector = hook::pattern("8B 44 24 04 3D 20 03 00 00 56 57 7F ? 74 ? 3D 00 02 00 00 74 ? 3D 80 02 00 00 75 ? BE ? ? ? ? BF ? ? ? ? B9 4A 00 00 00 F3 A5");
    auto hudFrame = hook::pattern("83 EC 3C 53 8B D8 8B 43 2C 50 E8 ? ? ? ? 8B C8 83 C4 04 85 C9 89 4C 24 1C 0F 84 ? ? ? ? 8B 81 78 08 00 00");
    if (hudSelector.size() == 1 && hudFrame.size() == 1)
    {
        SelectHudLayout = reinterpret_cast<int(__cdecl*)(int)>(hudSelector.get_first());
        static auto HudLayoutFrame = safetyhook::create_mid(hudFrame.get_first(), [](SafetyHookContext&)
        {
            ApplyPendingHudCoordinates();
        });
        if (!HudLayoutFrame) SelectHudLayout = nullptr;
    }
    else
        OutputDebugStringA("The Punisher Widescreen Fix: live HUD layout signatures unavailable.\n");

    //Resolution
    if (wfp::PatchPreflight("The Punisher resolution: signatures unavailable; feature disabled.\n",
    {
        { "89 1D ? ? ? ? A3 ? ? ? ? 88 0D", 1 },
        { "C7 04 24 80 02 00 00", 1 },
        { "C7 44 24 04 E0 01 00 00", 1 },
    }))
    {
        pattern = hook::pattern("89 1D ? ? ? ? A3 ? ? ? ? 88 0D");
        static int32_t* dword_913250 = *pattern.get_first<int32_t*>(2);
        static int32_t* dword_913254 = *pattern.get_first<int32_t*>(7);
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_913250 = Screen.Width;
                *dword_913254 = Screen.Height;
            }
        }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 04 24 80 02 00 00"); //0x4DD6CB
        injector::MakeNOP(pattern.get_first(-2), 2, true);
        ProtectedGameRef<int32_t>{pattern.get_first<int32_t>(3)} = Screen.Width;

        pattern = hook::pattern("C7 44 24 04 E0 01 00 00"); //0x4DD6ED
        injector::MakeNOP(pattern.get_first(-2), 2, true);
        ProtectedGameRef<int32_t>{pattern.get_first<int32_t>(4)} = Screen.Height;

    }

    //Hud, menu and presets

    //fixing only menu and startup stuff, which makes the game stuck in an infinite loop
    //this code doesn't work anymore because patterns don't find 0x46d91b and 0x4b7cbb calls for some reason and retXmap indices are wrong
    //uint32_t retXIncludes[] = { 25, 100, 101, 102, 103, 105, /*106,*/ 107, 130, 140, 142, 144, 145, 146, 149, 150, 151, 158, /*180,*/ 215, 216, 56, 99 };
    uint32_t retXIncludes[] = { 25, 100 }; // 25 - hud, 100 - preset, 180 - mouse, 215 - weapons overlay preset, 106 - weapons overlay scale
    //int32_t retYIncludes[] = { 24, 108, 107, 109, 110, 111, 112, 113, 114, 144, 146, 149, 150, 151, 152, 153, 154, 161, /*187,*/ 223, 224 };
    uint32_t retYIncludes[] = { 24, 108 }; // 24 - hud, 108 - preset, 187 - mouse, 223 - weapons overlay preset, 112+113 - weapons overlay scale

    pattern = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 50 6A 01 6A 03 E8 ? ? ? ? 8B 15");
    auto sub_510A00 = injector::GetBranchDestination(pattern.get_first(6), true);
    auto sub_510A10 = injector::GetBranchDestination(pattern.get_first(0), true);

    pattern = hook::pattern("E8 ? ? ? ?");
    for (size_t i = 0, j = 0, k = 0; i < pattern.size(); ++i)
    {
        auto addr = pattern.get(i).get<uint32_t>(0);
        auto dest = injector::GetBranchDestination(addr, true);

        if (dest == sub_510A00)
        {
#ifdef _LOG
            retXmap.insert(std::pair<uintptr_t, uint32_t>((uintptr_t)addr, j));
#endif
            if (!(std::end(retXIncludes) == std::find(std::begin(retXIncludes), std::end(retXIncludes), j)))
                injector::MakeCALL(addr, retX, true);
            ++j;
        }
        else if (dest == sub_510A10)
        {
#ifdef _LOG
            retYmap.insert(std::pair<uintptr_t, uint32_t>((uintptr_t)addr, k));
#endif
            if (!(std::end(retYIncludes) == std::find(std::begin(retYIncludes), std::end(retYIncludes), k)))
                injector::MakeCALL(addr, retY, true);
            ++k;
        }
    }

    //so instead just manual patch
    //pattern = hook::pattern("E8 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 6A 01"); //X - 25, Y - 24
    //injector::MakeCALL(pattern.get_first(0), retX, true);
    //injector::MakeCALL(pattern.get_first(10), retY, true);
    //pattern = hook::pattern("E8 ? ? ? ? 50 8D 44 24 38 68 ? ? ? ? 50"); //X - 100, Y - 108
    //injector::MakeCALL(pattern.get_first(0), retX, true);
    //injector::MakeCALL(pattern.get_first(-6), retY, true);

    //HUD
    if (wfp::PatchPreflight("The Punisher HUD and menus: signatures unavailable; feature disabled.\n",
    {
        { "68 ? ? ? ? 8B CF E8 ? ? ? ? 84 C0 ? ? 56 8B 74 24 08", 1 },
        { "68 ? ? ? ? 8B CF 83 C6 08 E8", 1 },
        { "89 0D ? ? ? ? 68 ? ? ? ? 8D", 1 },
        { "89 81 ? ? ? ? 8D 0C 24 E8 ? ? ? ? 8B 15", 1 },
        { "89 82 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 6B C9 2C", 1 },
        { "89 81 ? ? ? ? 8D 0C 24 E8 ? ? ? ? 8B 0D", 1 },
        { "8D 4C 24 04 89 82 ? ? ? ? E8 ? ? ? ? 84 C0", 1 },
        { "A2 ? ? ? ? 8A 44 24 10 88 0D", 1 },
        { "57 8B 7C 24 08 57 E8 ? ? ? ? 83 C4 04 85 C0 7D 7E A1 ? ? ? ? 55 56 8B F0 6B F6 2C", 1 },
        { "8B 54 24 34 51 52 50 E8 ? ? ? ? 83 C4 38 C3", 1 },
    }))
    {
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
        ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(0)} = 0x56;
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
        ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(0)} = 0x56;
        injector::MakeInline<HudHook>(pattern.get_first(1), pattern.get_first(7));


        //Menu
        pattern = hook::pattern("89 0D ? ? ? ? 68 ? ? ? ? 8D"); //0x477E26
        static uint32_t* dword_73DF80 = *pattern.get_first<uint32_t*>(2);
        static auto dword_73DFA4 = *hook::get_pattern<uint32_t>("89 81 ? ? ? ? 8D 0C 24 E8 ? ? ? ? 8B 15", 2);
        static auto dword_73DFA8 = *hook::get_pattern<uint32_t>("89 82 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 6B C9 2C", 2);
        static auto dword_73DFAC = *hook::get_pattern<uint32_t>("89 81 ? ? ? ? 8D 0C 24 E8 ? ? ? ? 8B 0D", 2);
        static auto dword_73DFB0 = *hook::get_pattern<uint32_t>("8D 4C 24 04 89 82 ? ? ? ? E8 ? ? ? ? 84 C0", 6);
        struct MenuHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto i = *dword_73DF80 * 44;
                auto x = *(int32_t*)(dword_73DFA4 + i);
                auto y = *(int32_t*)(dword_73DFA8 + i);
                auto w = *(int32_t*)(dword_73DFAC + i);
                auto h = *(int32_t*)&regs.eax;
                if (x == 0 && w == Screen.PresetWidth && h == (Screen.PresetHeight - y))
                {
                    //*(int32_t*)(dword_73DFA4 + i) = x;
                    //*(int32_t*)(dword_73DFA8 + i) = y;
                    *reinterpret_cast<int32_t*>(dword_73DFAC + i) = Screen.Width; //makes weapons overlay not being cut off by preset
                    *(int32_t*)&regs.eax = Screen.Height;
                }
                *dword_73DF80 = regs.ecx;
            }
        }; injector::MakeInline<MenuHook>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern("A2 ? ? ? ? 8A 44 24 10 88 0D"); //0x510B6C
        static auto gTexColor = *pattern.get_first<uint32_t*>(1);
        static auto GetTexColor = []() -> uint32_t
        {
            return *gTexColor;
        };

        static auto SetTexColor = [](uint32_t argb)
        {
            *gTexColor = argb;
        };

        pattern = hook::pattern("57 8B 7C 24 08 57 E8 ? ? ? ? 83 C4 04 85 C0 7D 7E A1 ? ? ? ? 55 56 8B F0 6B F6 2C"); //50C750
        static auto GetTexID = (int32_t(__cdecl*)(const char* FileName)) pattern.get_first(0);


        auto DrawHook = [](int32_t texID, int32_t x, int32_t y, int32_t w, int32_t h, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int8_t a10, int8_t a11, int32_t a12, int8_t a13, int8_t a14)
        {
            if (x == 0 && y == 0 && w == Screen.Width && h == Screen.Height)
            {
                x = static_cast<int32_t>(Screen.fHudOffsetReal);
                //y = top;
                w = static_cast<int32_t>(Screen.fWidth - Screen.fHudOffsetReal - Screen.fHudOffsetReal);
                //h = Screen.PresetHeight;

                auto og = GetTexColor();
                SetTexColor(0xFF000000);
                hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
                hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
                SetTexColor(og);
            }
            else
            {
                if ((x == 0 && y == 0 && w == 640 && h == 360) || (x == 0 && y == 0 && w == 960 && h == 540) || (x == 0 && y == 540 && w == 960 && h == 540))
                {
                    x += static_cast<int32_t>(Screen.fHudOffsetReal);
                    w -= static_cast<int32_t>(Screen.fHudOffsetReal);

                    auto og = GetTexColor();
                    SetTexColor(0xFF000000);
                    hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
                    hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
                    SetTexColor(og);
                }
                else
                {
                    if ((x == 960 && y == 0 && w == 960 && h == 540) || (x == 960 && y == 540 && w == 960 && h == 540))
                    {
                        w -= static_cast<int32_t>(Screen.fHudOffsetReal);

                        auto og = GetTexColor();
                        SetTexColor(0xFF000000);
                        hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, 1, 1, a10, a11, a12, a13, a14);
                        hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, 1, 1, a10, a11, a12, a13, a14);
                        SetTexColor(og);
                    }
                }
            }
            return hb_519CA0.fun(texID, x, y, w, h, a6, a7, a8, a9, a10, a11, a12, a13, a14);
        };
        pattern = hook::pattern("8B 54 24 34 51 52 50 E8 ? ? ? ? 83 C4 38 C3"); //0x510C68
        hb_519CA0.fun = injector::MakeCALL(pattern.get_first(7), static_cast<void(__cdecl*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int8_t, int8_t, int32_t, int8_t, int8_t)>(DrawHook), true).get();

        // This call belongs only to the movie player; menu textures keep their framing.
    auto movieDraw = hook::pattern("8B 15 ? ? ? ? 50 6A 00 6A 00 52 E8 ? ? ? ? 6A 00 E8 ? ? ? ? 83 C4 3C");
    auto moviePlayer = hook::pattern("A1 ? ? ? ? 83 EC 08 85 C0 0F 84 ? ? ? ? 8D 44 24 0C 50 E8");
    if (hb_519CA0.fun && movieDraw.size() == 1 && moviePlayer.size() == 1)
    {
        static auto decoder = *moviePlayer.get_first<uint32_t**>(1);
        auto drawMovie = [](int texture, int x, int y, int width, int height,
            int sourceX, int sourceY, int sourceWidth, int sourceHeight,
            char flipX, char flipY, int stage, int flags)
        {
            using Draw = int(__cdecl*)(int, int, int, int, int, int, int,
                int, int, char, char, int, int, char);
            const auto draw = reinterpret_cast<Draw>(hb_519CA0.fun);
            if (PunMenuLayout::Drawing)
            {
                return draw(texture, x, y, width, height, sourceX, sourceY,
                    sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
            }
            const auto video = *decoder;
            if (!video || !video[0] || !video[1] || Screen.Width <= 0 || Screen.Height <= 0)
                return draw(texture, x, y, width, height, sourceX, sourceY,
                    sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);

            static const auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
            float scaleX = Screen.fWidth / static_cast<float>(video[0]);
            float scaleY = Screen.fHeight / static_cast<float>(video[1]);
            if (mode != 2)
                scaleX = scaleY = mode == 1 ? std::max(scaleX, scaleY) : std::min(scaleX, scaleY);
            width = std::max(1, static_cast<int>(std::lround(video[0] * scaleX)));
            height = std::max(1, static_cast<int>(std::lround(video[1] * scaleY)));
            x = (Screen.Width - width) / 2;
            y = (Screen.Height - height) / 2;

            const auto colour = GetTexColor();
            SetTexColor(0xFF000000);
            if (x > 0)
            {
                draw(texture, 0, 0, x, Screen.Height, sourceX, sourceY,
                    sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
                draw(texture, x + width, 0, Screen.Width - x - width, Screen.Height,
                    sourceX, sourceY, sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
            }
            if (y > 0)
            {
                draw(texture, 0, 0, Screen.Width, y, sourceX, sourceY,
                    sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
                draw(texture, 0, y + height, Screen.Width, Screen.Height - y - height,
                    sourceX, sourceY, sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
            }
            SetTexColor(colour);
            return draw(texture, x, y, width, height, sourceX, sourceY,
                sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
        };
        injector::MakeCALL(movieDraw.get_first(12), +drawMovie, true);
    }
    else
    {
        OutputDebugStringA("The Punisher Widescreen Fix: movie layout signatures unavailable.\n");
    }

    //struct WeaponsOverlayHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        regs.eax = *(uint32_t*)(regs.esi + regs.edi + 0x230);
    //        bWeapOverlay = true;
    //    }
    //}; injector::MakeInline<WeaponsOverlayHook>(0x47D2DA, 0x47D2DA+7);

    }

    if (wfp::PatchPreflight("The Punisher overlays: signatures unavailable; feature disabled.\n",
    {
        { "6A 00 6A 00 B9 ? ? ? ? C7 05", 2 },
        { "75 ? E8 ? ? ? ? 8B 4C 24 28 89 01 8B CB E8 ? ? ? ? 89 45 00", 1 },
        { "D8 0D ? ? ? ? D9 5C 24 18 E8 ? ? ? ? 89", 1 },
        { "8B 8F 8C 00 00 00 50 53 53 51", 1 },
    }))
    {
        pattern = hook::pattern("6A 00 6A 00 B9 ? ? ? ? C7 05"); // 0x47C872 0x47C8C2
        ProtectedGameRef<uint8_t>{pattern.count_hint(2).get(0).get<uint8_t>(1)} = 1; // makes weapons screen stretched
        ProtectedGameRef<uint8_t>{pattern.count_hint(2).get(1).get<uint8_t>(1)} = 1;

        pattern = hook::pattern("75 ? E8 ? ? ? ? 8B 4C 24 28 89 01 8B CB E8 ? ? ? ? 89 45 00"); //0x5266C4
        struct WeaponsOverlayHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                **(int32_t**)(regs.esp + 0x28) = static_cast<int32_t>(static_cast<float>(**(int32_t**)(regs.esp + 0x28)) * (Screen.fWidth / (480.0f * Screen.fAspectRatio)));
                **(int32_t**)(regs.esp + 0x2C) = static_cast<int32_t>(static_cast<float>(**(int32_t**)(regs.esp + 0x2C)) * Screen.fHeight / 448.0f);

                **(int32_t**)(regs.esp + 0x28) = **(int32_t**)(regs.esp + 0x28) + static_cast<int32_t>(Screen.fHudOffsetReal);
            }
        };
        injector::MakeNOP(pattern.get_first(0), 2, true); //weapons overlay preset
        injector::MakeInline<WeaponsOverlayHook2>(pattern.get_first(23), pattern.get_first(23 + 14)); //0x5266DB, 0x5266E9

        pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 18 E8 ? ? ? ? 89"); //0x47D10E
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fHUDScaleX; //weapons overlay scale
        //pattern = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B 15 ? ? ? ? 50 6A"); //0x557FC8
        //ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fHUDScaleX; //videos scale

        pattern = hook::pattern("8B 8F 8C 00 00 00 50 53 53 51"); //0x48ADB2
        struct BackgroundHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = *(uint32_t*)(regs.edi + 0x8C);
                auto str = (char*)((*(uintptr_t*)(regs.edi)) + 0x30);
                static const char* menuBackgrounds[] = { "options_bg", "bf_detail_bg", "pc_controls_menu_bg", "gs_menu_bg", "av_menu_bg" };

                for (size_t i = 0; i < std::size(menuBackgrounds); ++i)
                {
                    if (memstr(str, menuBackgrounds[i], 0x60) != NULL)
                    {
                        regs.eax += 1;
                        break;
                    }
                }
            }
        }; injector::MakeInline<BackgroundHook>(pattern.get_first(0), pattern.get_first(6));


    }

    if (bSkipIntro)
    {
        if (wfp::PatchPreflight("The Punisher intro skip: signatures unavailable; feature disabled.\n",
        {
            { "E8 ? ? ? ? 68 00 00 40 3F 68 00 00", 5 },
        }))
        {
            pattern = hook::pattern("E8 ? ? ? ? 68 00 00 40 3F 68 00 00"); // 0x4DD5B9
            injector::MakeNOP(pattern.count_hint(5).get(3).get<void*>(0), 5, true);
        }


    }

    // The main world projection shares its builder with portrait, frontend and
    // debug views. Keep those other callers at their own intended framing.
    if (wfp::PatchPreflight("The Punisher gameplay FOV: signatures unavailable; feature disabled.\n",
    {
        { "8B 15 ? ? ? ? 8B 82 00 02 00 00 8B 15 ? ? ? ? 51 6A 00 68 00 00 7A 44 52 8D 48 1C 51 83 C0 28 50 E8 ? ? ? ?", 1 },
        { "A0 ? ? ? ? 83 EC 50 84 C0", 1 },
    }))
    {
        auto worldProjection = hook::pattern("8B 15 ? ? ? ? 8B 82 00 02 00 00 8B 15 ? ? ? ? 51 6A 00 68 00 00 7A 44 52 8D 48 1C 51 83 C0 28 50 E8 ? ? ? ?");
        static const auto worldReturn = worldProjection.size() == 1
            ? reinterpret_cast<uintptr_t>(worldProjection.get_first(40)) : uintptr_t{};
        if (!worldReturn)
            OutputDebugStringA("The Punisher Widescreen Fix: world camera projection unavailable.\n");
        pattern = hook::pattern("A0 ? ? ? ? 83 EC 50 84 C0"); // 0x515890
        static auto byte_9132CC = *pattern.get_first<uint8_t*>(1);
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *byte_9132CC;
                if (!worldReturn || *reinterpret_cast<uintptr_t*>(regs.esp) != worldReturn)
                    return;
                auto& fov = *(float*)(regs.esp + 0x0C);
                if (std::isfinite(fov) && fov > 0.0f && fov < 180.0f)
                {
                    const float factor = std::isfinite(fFOVFactor) ? std::clamp(fFOVFactor, 0.5f, 2.0f) : 1.0f;
                    const float corrected = AdjustFOV(fov, Screen.fAspectRatio) * factor;
                    if (std::isfinite(corrected) && corrected > 0.0f)
                        fov = std::min(corrected, 179.0f);
                }
            }
        }; injector::MakeInline<FOVHook>(pattern.get_first(0));

    }

    PunisherWindow::Init();
    PunMenuLayout::Init(Screen.Width, Screen.Height);
    PunMenu::Init();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        D3D8Compatibility::Initialize(false, false, []()
        {
            return PunisherFrameLimit.load(std::memory_order_relaxed);
        });
        WindowedModeWrapper::EnableDpiAwareness();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("89 1D ? ? ? ? A3 ? ? ? ? 88 0D"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
