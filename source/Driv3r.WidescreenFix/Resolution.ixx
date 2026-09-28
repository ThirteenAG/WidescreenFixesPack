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

std::filesystem::path GetResolutionConfigPath()
{
    return GetExeModulePath() / "Saves" / "RES.cfg";
}

struct ResolutionConfig
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t refreshrate = 0;
};

ResolutionConfig GetSavedResolution()
{
    ResolutionConfig cfg;
    auto path = GetResolutionConfigPath();

    std::ifstream file(path);
    if (!file.is_open())
        return cfg;

    std::string line;
    while (std::getline(file, line))
    {
        auto parse = [&](const char* key, uint32_t& out)
        {
            if (line.rfind(key, 0) == 0)
            {
                auto pos = line.find('=');
                if (pos != std::string::npos)
                    out = static_cast<uint32_t>(std::stoul(line.substr(pos + 1)));
            }
        };

        parse("width", cfg.width);
        parse("height", cfg.height);
        parse("refreshrate", cfg.refreshrate);
    }

    return cfg;
}

void SaveResolution(uint32_t width, uint32_t height, uint32_t refreshrate)
{
    auto path = GetResolutionConfigPath();

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open())
        return;

    file << "width = " << width << '\n'
        << "height = " << height << '\n'
        << "refreshrate = " << refreshrate << '\n';
}

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

SafetyHookInline shEnumAdapters = {};
HRESULT __stdcall EnumAdapters(AdapterList* list, IDirect3D9* d3d)
{
    auto hr = shEnumAdapters.unsafe_stdcall<HRESULT>(list, d3d);
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

const D3DDISPLAYMODE* GetListMode(uint32_t index)
{
    if (!GameModeList || !Adapters || index >= GameModeList->count)
        return nullptr;

    const auto& entry = GameModeList->entries[index];
    if (entry.adapter >= Adapters->count || entry.mode >= Adapters->adapters[entry.adapter].modeCount)
        return nullptr;

    return &Adapters->adapters[entry.adapter].modes[entry.mode];
}

// Saved resolution, or the desktop resolution, taken from the listed modes so that the game finds its index
D3DDISPLAYMODE GetDefaultMode()
{
    auto [DesktopResW, DesktopResH] = GetDesktopRes();
    auto config = GetSavedResolution();

    auto it = std::find_if(AdapterModes.begin(), AdapterModes.end(), [](const auto& modes) { return !modes.empty(); });
    if (it == AdapterModes.end())
        return { static_cast<UINT>(DesktopResW), static_cast<UINT>(DesktopResH), 60, D3DFMT_X8R8G8B8 };
    const auto& modes = *it;

    auto find = [&](uint32_t w, uint32_t h, uint32_t refreshrate) -> const D3DDISPLAYMODE*
    {
        const D3DDISPLAYMODE* best = nullptr;
        for (const auto& mode : modes)
        {
            if (mode.Width != w || mode.Height != h)
                continue;
            if (mode.RefreshRate == refreshrate)
                return &mode;
            if (!best || mode.RefreshRate > best->RefreshRate)
                best = &mode;
        }
        return best;
    };

    if (auto mode = find(config.width, config.height, config.refreshrate))
        return *mode;
    if (auto mode = find(DesktopResW, DesktopResH, 0))
        return *mode;
    return modes.back();
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

SafetyHookInline shSetMode = {};
void __fastcall SetMode(uint8_t* _this, void* edx, int32_t width, int32_t height, int32_t index)
{
    index = ResolveModeIndex(width, height, index);
    shSetMode.unsafe_thiscall(_this, width, height, index);

    // A failed Reset restores the previous mode, keep the viewports and the current index in sync with it
    auto& currentIndex = *reinterpret_cast<int32_t*>(_this + 0x1098);
    if (currentIndex == index && RendererModeIndex != index)
    {
        currentIndex = RendererModeIndex;
        *reinterpret_cast<int32_t*>(_this + 0x105C) = BackbufferWidth;
        *reinterpret_cast<int32_t*>(_this + 0x1060) = BackbufferHeight;
    }
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

            auto pattern = hook::pattern("33 C0 39 05 ? ? ? ? A3 ? ? ? ? A2 ? ? ? ? 75");
            GameModeList = reinterpret_cast<ModeList*>(*pattern.get_first<uintptr_t>(9) - offsetof(ModeList, count));

            //curated 32 bit mode list, the same every time the game builds its list from it
            pattern = hook::pattern("6A FF 64 A1 ? ? ? ? 68 ? ? ? ? 50 8B 44 24 14");
            shEnumAdapters = safetyhook::create_inline(pattern.get_first(), EnumAdapters);

            //uncap resolutions, the mode list is already limited
            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 57 8B DE E8 ? ? ? ? 85 C0"); //54A5A0
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), 0, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), 0, true);

            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 85"); //57F665
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), 0, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), 0, true);

            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 50 33 DB E8"); //5C9288
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), 0, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), 0, true);

            //default to desktop res
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            pattern = hook::pattern("56 57 56 56 68 ? ? ? ? 68 ? ? ? ? 56 56"); //576C6B
            injector::WriteMemory(pattern.get_first<int32_t*>(5), DesktopResH, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(10), DesktopResW, true);

            //startup resolution (before profile loading)
            pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? ? ? E8"); //57F6DF
            struct DefaultResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(D3DDISPLAYMODE*)(regs.esp + 0x28) = GetDefaultMode();
                }
            }; injector::MakeInline<DefaultResHook>(pattern.get_first(0), pattern.get_first(32));

            //default entry of the resolution menu
            pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 85 C0 7D"); //54A7DF
            struct MenuDefaultResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(D3DDISPLAYMODE*)(regs.esp + 0x04) = GetDefaultMode();
                }
            }; injector::MakeInline<MenuDefaultResHook>(pattern.get_first(0), pattern.get_first(16));

            //stale mode indices
            pattern = hook::pattern("56 57 8B 7C 24 14 8B F1 39 BE 98 10 00 00");
            shSetMode = safetyhook::create_inline(pattern.get_first(), SetMode);

            //resolution switch
            pattern = hook::pattern("89 8B 80 01 00 00"); //0x5C938F, 0x5C947E
            struct GetResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ebx + 0x180) = regs.ecx;

                    auto w = *(uint32_t*)(regs.ebx + 0x160);
                    auto h = *(uint32_t*)(regs.ebx + 0x164);
                    auto refreshrate = *(uint32_t*)(regs.ebx + 0x190);

                    SaveResolution(w, h, refreshrate);
                    onResChange().executeAll(w, h);
                }
            };
            injector::MakeInline<GetResHook>(pattern.count(2).get(0).get<void*>(0), pattern.count(2).get(0).get<void*>(6));
            injector::MakeInline<GetResHook>(pattern.count(2).get(1).get<void*>(0), pattern.count(2).get(1).get<void*>(6));

            onResChange() += [](int Width, int Height)
            {
                auto [DesktopResW, DesktopResH] = GetDesktopRes();
                static tagRECT REKT;
                REKT.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)Width / 2.0f));
                REKT.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)Height / 2.0f));
                REKT.right = (LONG)Width;
                REKT.bottom = (LONG)Height;
                SetWindowPos(hWnd, NULL, REKT.left, REKT.top, REKT.right, REKT.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
            };
        };
    }
} Resolution;
