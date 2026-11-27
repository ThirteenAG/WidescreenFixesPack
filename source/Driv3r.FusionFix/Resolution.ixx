module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <charconv>

export module Resolution;

import ComVars;
import Window;

// The resolution menu is built from the game's mode list, which refers to the modes stored per adapter
constexpr uint32_t MaxListModes = 100;
constexpr uint32_t MaxAdapterModes = 500;

std::vector<std::vector<D3DDISPLAYMODE>> AdapterModes;

bool bForceMaxRefreshRate = true;
int32_t nMinResX = 0;
int32_t nMinResY = 0;
int32_t nMaxResX = 0;
int32_t nMaxResY = 0;

struct ResolutionConfig
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t refreshRate = 0;
};

ResolutionConfig GetSavedResolution()
{
    const auto value = FixSettings().Get("MAIN.DisplayMode");
    const char* first = value.data();
    const char* last = first + value.size();
    ResolutionConfig config;
    for (auto field : { &config.width, &config.height, &config.refreshRate })
    {
        const auto result = std::from_chars(first, last, *field);
        if (result.ec != std::errc{})
            return {};
        if (field == &config.refreshRate)
            return result.ptr == last ? config : ResolutionConfig{};
        if (result.ptr == last || *result.ptr != 'x')
            return {};
        first = result.ptr + 1;
    }
    return {};
}

void SaveResolution(uint32_t width, uint32_t height, uint32_t refreshRate)
{
    auto value = std::to_string(width) + 'x' + std::to_string(height) + 'x' + std::to_string(refreshRate);
    if (value != FixSettings().Get("MAIN.DisplayMode"))
        FixSettings().Set("MAIN.DisplayMode", value);
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

    if (modes.size() <= MaxListModes)
    {
        std::reverse(modes.begin(), modes.end());
        return modes;
    }

    const auto saved = GetSavedResolution();
    D3DDISPLAYMODE desktop{};
    d3d->GetAdapterDisplayMode(adapter, &desktop);

    // The list is sorted from highest to lowest refresh within each resolution.
    const auto preferred = [&](uint32_t width, uint32_t height, uint32_t refreshRate)
    {
        const auto exact = std::find_if(modes.begin(), modes.end(), [&](const auto& mode)
        {
            return mode.Width == width && mode.Height == height && mode.RefreshRate == refreshRate;
        });
        return exact != modes.end() ? exact : std::find_if(modes.begin(), modes.end(), [&](const auto& mode)
        {
            return mode.Width == width && mode.Height == height;
        });
    };
    const auto selected = preferred(saved.width, saved.height, saved.refreshRate);
    const auto desktopMode = preferred(desktop.Width, desktop.Height, 0);
    size_t available = MaxListModes - (selected != modes.end()) - (desktopMode != modes.end() && desktopMode != selected);

    std::vector<D3DDISPLAYMODE> result;
    result.reserve(MaxListModes);
    for (auto it = modes.begin(); it != modes.end(); ++it)
    {
        if (it == selected || it == desktopMode)
            result.push_back(*it);
        else if (available)
        {
            result.push_back(*it);
            --available;
        }
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
    if (!GameModeList.is_initialized() || !Adapters || index >= GameModeList.get().count)
        return nullptr;

    const auto& entry = GameModeList.get().entries[index];
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

    if (auto mode = find(config.width, config.height, config.refreshRate))
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
    for (uint32_t i = 0; GameModeList.is_initialized() && i < GameModeList.get().count; ++i)
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

void* SetModeTarget = nullptr;
SafetyHookInline shSetMode = {};
bool ChangingMode = false, ModeBuilt = false;
void SaveMode(const D3DPRESENT_PARAMETERS& present, int index)
{
    // Windowed presentation uses refresh 0; keep the player's selected display
    // mode for a later fullscreen start, including its chosen refresh rate.
    auto selected = present.Windowed ? GetListMode(index) : nullptr;
    if (selected) SaveResolution(selected->Width, selected->Height, selected->RefreshRate);
    else SaveResolution(present.BackBufferWidth, present.BackBufferHeight, present.FullScreen_RefreshRateInHz);
}
void PublishMode(void* context)
{
    auto present = reinterpret_cast<D3DPRESENT_PARAMETERS*>(static_cast<uint8_t*>(context) + 0x160);
    if (ChangingMode) { ModeBuilt = true; return; }
    if (present->BackBufferWidth && present->BackBufferHeight)
    {
        SaveMode(*present, Driv3rWindow::requestedIndex);
        onResChange().executeAll(present->BackBufferWidth, present->BackBufferHeight);
    }
}
void __fastcall SetMode(uint8_t* _this, void* edx, int32_t width, int32_t height, int32_t index)
{
    index = ResolveModeIndex(width, height, index);
    // The game creates viewports inside SetMode, before it resets the device.
    // Preserve the selected mode index, but build the viewports at the borderless client size.
    if (Driv3rWindow::mode == 2)
    {
        int clientWidth = width;
        int clientHeight = height;
        if (Driv3rWindow::Dimensions(clientWidth, clientHeight))
        {
            width = clientWidth;
            height = clientHeight;
        }
    }
    ChangingMode = true;
    ModeBuilt = false;
    shSetMode.unsafe_thiscall(_this, width, height, index);
    ChangingMode = false;

    // Publish the dimensions accepted by Reset, including its restored mode on failure.
    if (Driv3rWindow::SynchronizeViewport(_this, index, RendererModeIndex, BackbufferWidth, BackbufferHeight, ModeBuilt))
    {
        auto present = reinterpret_cast<D3DPRESENT_PARAMETERS*>(Renderer.get_ptr() + 0x160);
        SaveMode(*present, RendererModeIndex);
        onResChange().executeAll(BackbufferWidth, BackbufferHeight);
    }
}

bool ChangeWindowMode(int width, int height)
{
    auto* manager = static_cast<uint8_t*>(GraphicsManager.get());
    if (!manager || (*reinterpret_cast<void***>(manager))[34] != SetModeTarget) return false;
    const auto* present = reinterpret_cast<const D3DPRESENT_PARAMETERS*>(Renderer.get_ptr() + 0x160);
    if (width <= 0 || height <= 0)
    {
        width = present->BackBufferWidth;
        height = present->BackBufferHeight;
    }
    if (width <= 0 || height <= 0) return false;
    const int index = RendererModeIndex;
    *reinterpret_cast<int32_t*>(manager + 0x1098) = -1;
    SetMode(manager, nullptr, width, height, index);
    return ModeBuilt && (present->Windowed != FALSE) == (Driv3rWindow::mode != 0);
}

// The listed mode stays selected; the renderer skips a rebuild for its own current mode, so that is forgotten first
bool ResizeWindowed(int width, int height)
{
    auto* manager = static_cast<uint8_t*>(GraphicsManager.get());
    if (!manager || (*reinterpret_cast<void***>(manager))[34] != SetModeTarget) return false;
    const auto* present = reinterpret_cast<const D3DPRESENT_PARAMETERS*>(Renderer.get_ptr() + 0x160);
    const int index = RendererModeIndex;
    *reinterpret_cast<int32_t*>(manager + 0x1098) = -1;
    RendererModeIndex = -1;
    SetMode(manager, nullptr, width, height, index);
    if (RendererModeIndex == -1)
        RendererModeIndex = index;
    return ModeBuilt && present->Windowed && present->BackBufferWidth == UINT(width) && present->BackBufferHeight == UINT(height);
}

class Resolution
{
public:
    Resolution()
    {
        GraphicsManager.SetAddress(*hook::get_pattern<void**>("8B 0D ? ? ? ? 8B 11 50 8B 43 48 50 8B 06 50 FF 92 88 00 00 00", 2));
        WFP::onInitEvent() += []()
        {

            bForceMaxRefreshRate = FixSettings().Bool("MAIN.ForceMaxRefreshRate") != 0;
            nMinResX = FixSettings().Int("MAIN.MinResX");
            nMinResY = FixSettings().Int("MAIN.MinResY");
            nMaxResX = FixSettings().Int("MAIN.MaxResX");
            nMaxResY = FixSettings().Int("MAIN.MaxResY");
            Driv3rWindow::onPrepared = PublishMode;

            auto pattern = hook::pattern("33 C0 39 05 ? ? ? ? A3 ? ? ? ? A2 ? ? ? ? 75");
            GameModeList.SetAddress(*pattern.get_first<uintptr_t>(9) - offsetof(ModeList, count));

            //curated 32 bit mode list, the same every time the game builds its list from it
            pattern = hook::pattern("6A FF 64 A1 ? ? ? ? 68 ? ? ? ? 50 8B 44 24 14");
            shEnumAdapters = safetyhook::create_inline(pattern.get_first(), EnumAdapters);

            //uncap resolutions, the mode list is already limited
            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 57 8B DE E8 ? ? ? ? 85 C0"); //54A5A0
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 0), INT_MAX, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 5), INT_MAX, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 10), 0, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 15), 0, true);

            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 85"); //57F665
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 0), INT_MAX, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 5), INT_MAX, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 10), 0, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 15), 0, true);

            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 50 33 DB E8"); //5C9288
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 0), INT_MAX, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 5), INT_MAX, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 10), 0, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1 + 15), 0, true);

            //no bit depth after the menu's resolution label: every listed mode is 32 bit, and the suffix wraps under long labels
            pattern = hook::pattern("8B 76 0C 8D 46 EA 83 C4 14 83 F8 0D 77"); //54A8CE
            injector::WriteMemory<uint8_t>(pattern.get_first(12), 0xEB, true);

            //default to desktop res
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            pattern = hook::pattern("56 57 56 56 68 ? ? ? ? 68 ? ? ? ? 56 56"); //576C6B
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(5), DesktopResH, true);
            injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(10), DesktopResW, true);

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

            // Use the current list entry when a profile selects a matching mode.
            // Its stored index and display label may have come from a different list.
            pattern = hook::pattern("85 C0 7D ? E8 ? ? ? ? B9 15 00 00 00 8B F0 8B FB F3 A5 8D 73 44");
            static auto profileMode = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                struct MenuMode
                {
                    int32_t index;
                    wchar_t label[32];
                    D3DDISPLAYMODE mode;
                };
                static_assert(sizeof(MenuMode) == 0x54);
                struct MenuModes
                {
                    int32_t count;
                    const MenuMode* entries;
                };
                const auto index = static_cast<int32_t>(regs.eax);
                const auto& modes = *reinterpret_cast<const MenuModes*>(regs.edi);
                if (index >= 0 && index < modes.count)
                    *reinterpret_cast<MenuMode*>(regs.ebx) = modes.entries[index];
            });

            //stale mode indices
            pattern = hook::pattern("56 57 8B 7C 24 14 8B F1 39 BE 98 10 00 00");
            SetModeTarget = pattern.get_first();
            shSetMode = safetyhook::create_inline(SetModeTarget, SetMode);
            Driv3rWindow::onModeChange = ChangeWindowMode;
            Driv3rWindow::onResize = ResizeWindowed;

            //resolution switch
            pattern = hook::pattern("89 8B 80 01 00 00"); //0x5C938F, 0x5C947E
            struct GetResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ebx + 0x180) = regs.ecx;

                    PublishMode(reinterpret_cast<void*>(regs.ebx));
                }
            };
            injector::MakeInline<GetResHook>(pattern.count(2).get(0).get<void*>(0), pattern.count(2).get(0).get<void*>(6));
            injector::MakeInline<GetResHook>(pattern.count(2).get(1).get<void*>(0), pattern.count(2).get(1).get<void*>(6));

            onResChange() += [](int Width, int Height)
            {
                if (Driv3rWindow::Place(Width, Height)) return;
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
