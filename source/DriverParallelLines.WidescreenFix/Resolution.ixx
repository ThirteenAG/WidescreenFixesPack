module;

#include <stdafx.h>
#include <d3d9.h>

export module Resolution;

import ComVars;

// The resolution menu is built from the game's mode list, which refers to the modes stored per adapter
constexpr uint32_t MaxListModes = 100;
constexpr uint32_t MaxAdapterModes = 500;

struct AdapterInfo
{
    IDirect3D9* d3d;
    UINT adapter;
    bool initialized;
    alignas(4) std::byte identifier[1100]; // D3DADAPTER_IDENTIFIER9
    D3DDISPLAYMODE modes[MaxAdapterModes];
    uint32_t modeCount;
    D3DFORMAT formats[20];
    uint32_t formatCount;
};
static_assert(offsetof(AdapterInfo, modes) == 0x458 && sizeof(AdapterInfo) == 0x23F0);

struct AdapterList
{
    bool initialized;
    IDirect3D9* d3d;
    AdapterInfo* adapters;
    uint32_t count;
};

// The game, its resolution menu and saved profiles refer to modes by their index in this list
struct ModeList
{
    struct
    {
        UINT adapter;
        UINT mode;
    } entries[MaxListModes];
    uint32_t count;
    bool built;
};
static_assert(offsetof(ModeList, count) == 800);

AdapterList* Adapters = nullptr;
ModeList* GameModeList = nullptr;
std::vector<std::vector<D3DDISPLAYMODE>> AdapterModes;

bool bForceMaxRefreshRate = true;
int32_t nMinResX = 0;
int32_t nMinResY = 0;
int32_t nMaxResX = 0;
int32_t nMaxResY = 0;

bool IsSameResolution(const D3DDISPLAYMODE& a, const D3DDISPLAYMODE& b)
{
    return a.Width == b.Width && a.Height == b.Height;
}

// 32 bit modes only, largest resolutions first; the lowest resolutions are cut off when the list is full
std::vector<D3DDISPLAYMODE> BuildModeList(IDirect3D9* d3d, UINT adapter)
{
    const uint32_t minW = nMinResX > 0 ? nMinResX : 640;
    const uint32_t minH = nMinResY > 0 ? nMinResY : 480;
    const uint32_t maxW = nMaxResX > 0 ? nMaxResX : UINT_MAX;
    const uint32_t maxH = nMaxResY > 0 ? nMaxResY : UINT_MAX;

    std::vector<D3DDISPLAYMODE> modes;
    const UINT count = d3d->GetAdapterModeCount(adapter, D3DFMT_X8R8G8B8);
    for (UINT i = 0; i < count; ++i)
    {
        D3DDISPLAYMODE mode = {};
        if (FAILED(d3d->EnumAdapterModes(adapter, D3DFMT_X8R8G8B8, i, &mode)))
            continue;

        if (mode.Width >= minW && mode.Height >= minH && mode.Width <= maxW && mode.Height <= maxH)
            modes.push_back(mode);
    }

    std::sort(modes.begin(), modes.end(), [](const D3DDISPLAYMODE& a, const D3DDISPLAYMODE& b)
    {
        return std::tie(a.Width, a.Height, a.RefreshRate) > std::tie(b.Width, b.Height, b.RefreshRate);
    });

    // D3D9 reports some modes more than once, e.g. once per scaling mode
    modes.erase(std::unique(modes.begin(), modes.end(), [](const D3DDISPLAYMODE& a, const D3DDISPLAYMODE& b)
    {
        return IsSameResolution(a, b) && a.RefreshRate == b.RefreshRate;
    }), modes.end());

    if (bForceMaxRefreshRate)
        modes.erase(std::unique(modes.begin(), modes.end(), IsSameResolution), modes.end());

    std::vector<D3DDISPLAYMODE> result;
    for (auto it = modes.begin(); it != modes.end();)
    {
        auto next = std::find_if_not(it, modes.end(), [&](const D3DDISPLAYMODE& mode) { return IsSameResolution(mode, *it); });
        if (result.size() + std::distance(it, next) > MaxListModes)
            break;

        result.insert(result.end(), it, next);
        it = next;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

// 32 bit HD modes are dropped too, the back buffer is A8R8G8B8 with any of them
SafetyHookInline shEnumAdapters = {};
HRESULT __fastcall EnumAdapters(AdapterList* list, void* edx, IDirect3D9* d3d)
{
    auto hr = shEnumAdapters.unsafe_thiscall<HRESULT>(list, d3d);
    if (FAILED(hr) || !list->adapters)
        return hr;

    Adapters = list;
    AdapterModes.assign(list->count, {});

    for (uint32_t i = 0; i < list->count; ++i)
    {
        auto& info = list->adapters[i];
        if (!info.initialized || !info.d3d)
            continue;

        auto modes = BuildModeList(info.d3d, info.adapter);
        if (modes.empty())
            continue;

        std::copy(modes.begin(), modes.end(), info.modes);
        info.modeCount = static_cast<uint32_t>(modes.size());
        info.formats[0] = D3DFMT_X8R8G8B8;
        info.formatCount = 1;

        AdapterModes[i] = std::move(modes);
    }

    return hr;
}

// Builds the mode list from the modes above, the list is already limited
SafetyHookInline shFillModeList = {};
int __fastcall FillModeList(ModeList* list, void* edx, UINT adapter, UINT minW, UINT minH, UINT maxW, UINT maxH)
{
    GameModeList = list;
    return shFillModeList.unsafe_thiscall<int>(list, adapter, 0, 0, INT_MAX, INT_MAX);
}

const D3DDISPLAYMODE* GetListMode(uint32_t index)
{
    if (!GameModeList || !Adapters || index >= GameModeList->count)
        return nullptr;

    const auto& entry = GameModeList->entries[index];
    if (entry.adapter >= Adapters->count || entry.mode >= Adapters->adapters[entry.adapter].modeCount)
        return nullptr;

    return &Adapters->adapters[entry.adapter].modes[entry.mode];
}

// Desktop resolution, taken from the listed modes so that the game finds its index
D3DDISPLAYMODE GetDefaultMode()
{
    auto [DesktopResW, DesktopResH] = GetDesktopRes();

    auto it = std::find_if(AdapterModes.begin(), AdapterModes.end(), [](const auto& modes) { return !modes.empty(); });
    if (it == AdapterModes.end())
        return { static_cast<UINT>(DesktopResW), static_cast<UINT>(DesktopResH), GetDesktopRefreshRate(), D3DFMT_X8R8G8B8 };
    const auto& modes = *it;

    const D3DDISPLAYMODE* best = nullptr;
    for (const auto& mode : modes)
    {
        if (mode.Width == static_cast<UINT>(DesktopResW) && mode.Height == static_cast<UINT>(DesktopResH) && (!best || mode.RefreshRate > best->RefreshRate))
            best = &mode;
    }
    return best ? *best : modes.back();
}

// Menu entries and profiles store an index into the mode list, it no longer matches when the list changed since
int32_t ResolveModeIndex(int32_t width, int32_t height, int32_t index)
{
    if (auto mode = GetListMode(index); mode && mode->Width == static_cast<UINT>(width) && mode->Height == static_cast<UINT>(height))
        return index;

    int32_t best = -1;
    UINT bestRefreshRate = 0;
    for (uint32_t i = 0; GameModeList && i < GameModeList->count; ++i)
    {
        auto mode = GetListMode(i);
        if (mode && mode->Width == static_cast<UINT>(width) && mode->Height == static_cast<UINT>(height) && mode->RefreshRate >= bestRefreshRate)
        {
            best = static_cast<int32_t>(i);
            bestRefreshRate = mode->RefreshRate;
        }
    }
    return best >= 0 ? best : index;
}

// The new mode gets its size from the arguments, but its refresh rate from the mode at the index
SafetyHookInline shSetMode = {};
void* __fastcall SetMode(void* _this, void* edx, int32_t width, int32_t height, int32_t index)
{
    return shSetMode.unsafe_thiscall<void*>(_this, width, height, ResolveModeIndex(width, height, index));
}

class Resolution
{
public:
    Resolution()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            bForceMaxRefreshRate = iniReader.ReadInteger("MAIN", "ForceMaxRefreshRate", 1) != 0;
            nMinResX = iniReader.ReadInteger("MAIN", "MinResX", 0);
            nMinResY = iniReader.ReadInteger("MAIN", "MinResY", 0);
            nMaxResX = iniReader.ReadInteger("MAIN", "MaxResX", 0);
            nMaxResY = iniReader.ReadInteger("MAIN", "MaxResY", 0);

            //curated 32 bit mode list, the same every time the game builds its list from it
            auto pattern = hook::pattern("8B 44 24 04 53 55 56 8B F1 57 89 46 04 8B 08");
            shEnumAdapters = safetyhook::create_inline(pattern.get_first(), EnumAdapters);

            //uncap resolutions
            pattern = find_pattern("E8 ? ? ? ? 85 C0 0F 8C ? ? ? ? 8D 45", "E8 ? ? ? ? 85 C0 7C ? 33 C0 5D");
            shFillModeList = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), FillModeList);

            //default to desktop res, for new profiles, the startup mode and the default entry of the resolution menu
            pattern = hook::pattern("C7 46 4C ? ? ? ? C7 46 50 ? ? ? ? C7 46 54 ? ? ? ? C7 46 58 16 00 00 00"); //4C5077
            struct ProfileDefaultResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(D3DDISPLAYMODE*)(regs.esi + 0x4C) = GetDefaultMode();
                }
            }; injector::MakeInline<ProfileDefaultResHook>(pattern.get_first(0), pattern.get_first(28));

            pattern = hook::pattern("C7 45 E0 ? ? ? ? C7 45 E4 ? ? ? ? C7 45 E8 ? ? ? ? C7 45 EC 16 00 00 00"); //0x5EBA06
            struct StartupResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(D3DDISPLAYMODE*)(regs.ebp - 0x20) = GetDefaultMode();
                }
            }; injector::MakeInline<StartupResHook>(pattern.get_first(0), pattern.get_first(28));

            pattern = hook::pattern("C7 45 F0 00 04 00 00 C7 45 F4 00 03 00 00 C7 45 F8 3C 00 00 00 C7 45 FC 16 00 00 00"); //0x4C5932
            struct MenuDefaultResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(D3DDISPLAYMODE*)(regs.ebp - 0x10) = GetDefaultMode();
                }
            }; injector::MakeInline<MenuDefaultResHook>(pattern.get_first(0), pattern.get_first(28));

            //stale mode indices
            pattern = hook::pattern("55 8B EC 83 EC 10 56 8B F1 8B 4D 10 8D 86 ? ? ? ? 39 08");
            shSetMode = safetyhook::create_inline(pattern.get_first(), SetMode);

            //resolution switch
            pattern = hook::pattern("C6 01 01 33 C0 C2 0C 00"); //0x5E4A6A
            struct GetResHook1
            {
                void operator()(injector::reg_pack& regs)
                {
                    onResChange().executeAll(*BackbufferWidth, *BackbufferHeight);
                    *(uint8_t*)regs.ecx = 1;
                    regs.eax = 0;
                }
            }; injector::MakeInline<GetResHook1>(pattern.get_first(0));

            pattern = hook::pattern("89 8E 8C 01 00 00"); //0x5E4C73
            struct GetResHook2
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.esi + 0x18C) = regs.ecx;
                    onResChange().executeAll(*BackbufferWidth, *BackbufferHeight);
                }
            }; injector::MakeInline<GetResHook2>(pattern.get_first(0), pattern.get_first(6));
        };
    }
} Resolution;
