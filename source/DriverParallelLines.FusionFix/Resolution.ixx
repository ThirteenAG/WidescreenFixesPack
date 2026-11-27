module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module Resolution;

import ComVars;
import Window;

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
UINT ModeListAdapter = 0;
std::vector<std::vector<D3DDISPLAYMODE>> AdapterModes;
std::vector<std::vector<D3DDISPLAYMODE>> AvailableModes;

bool bForceMaxRefreshRate = true;
int32_t nMinResX = 0;
int32_t nMinResY = 0;
int32_t nMaxResX = 0;
int32_t nMaxResY = 0;

bool IsSameResolution(const D3DDISPLAYMODE& a, const D3DDISPLAYMODE& b)
{
    return a.Width == b.Width && a.Height == b.Height;
}

// Enumerate valid 32-bit modes from largest to smallest, with the fastest refresh first at each size.
std::vector<D3DDISPLAYMODE> EnumerateModes(IDirect3D9* d3d, UINT adapter)
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
    // Exact bounds can request a framed client size that the monitor does not advertise.
    if (modes.empty() && DPLWindow::mode == 1 && nMinResX > 0 && nMinResY > 0 && minW == maxW && minH == maxH)
        modes.push_back({ minW, minH, 0, D3DFMT_X8R8G8B8 });

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
    return modes;
}

std::vector<D3DDISPLAYMODE> BuildModeList(const std::vector<D3DDISPLAYMODE>& modes, UINT width = 0, UINT height = 0)
{
    if (modes.size() <= MaxListModes)
        return { modes.rbegin(), modes.rend() };
    std::vector<D3DDISPLAYMODE> result;
    result.reserve(MaxListModes);
    auto [desktopWidth, desktopHeight] = GetDesktopRes();
    const D3DDISPLAYMODE desktop{ static_cast<UINT>(desktopWidth), static_cast<UINT>(desktopHeight) };
    const D3DDISPLAYMODE selected{ width, height };
    const auto include = [&](const D3DDISPLAYMODE& size)
    {
        const auto mode = std::find_if(modes.begin(), modes.end(), [&](const auto& entry) { return IsSameResolution(entry, size); });
        if (mode != modes.end())
            result.push_back(*mode);
    };
    include(desktop);
    if (!IsSameResolution(selected, desktop))
        include(selected);

    // Reserve one selectable refresh at each size before adding the remaining refresh rates.
    for (size_t i = 0; i < modes.size() && result.size() < MaxListModes; ++i)
    {
        const auto& mode = modes[i];
        if ((!i || !IsSameResolution(mode, modes[i - 1])) &&
            !IsSameResolution(mode, desktop) && !IsSameResolution(mode, selected))
            result.push_back(mode);
    }
    for (size_t i = 1; i < modes.size() && result.size() < MaxListModes; ++i)
        if (IsSameResolution(modes[i], modes[i - 1]))
            result.push_back(modes[i]);

    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b)
    {
        return std::tie(a.Width, a.Height, a.RefreshRate) < std::tie(b.Width, b.Height, b.RefreshRate);
    });
    return result;
}

void SetAdapterModes(UINT adapter, std::vector<D3DDISPLAYMODE> modes)
{
    auto& info = Adapters->adapters[adapter];
    std::copy(modes.begin(), modes.end(), info.modes);
    info.modeCount = static_cast<uint32_t>(modes.size());
    info.formats[0] = D3DFMT_X8R8G8B8;
    info.formatCount = 1;
    AdapterModes[adapter] = std::move(modes);
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
    AvailableModes.assign(list->count, {});

    for (uint32_t i = 0; i < list->count; ++i)
    {
        auto& info = list->adapters[i];
        if (!info.initialized || !info.d3d)
            continue;

        auto& available = AvailableModes[i];
        available = EnumerateModes(info.d3d, info.adapter);
        if (available.empty())
            continue;
        SetAdapterModes(i, BuildModeList(available));
    }

    return hr;
}

// Builds the mode list from the modes above, the list is already limited
SafetyHookInline shFillModeList = {};
int __fastcall FillModeList(ModeList* list, void* edx, UINT adapter, UINT minW, UINT minH, UINT maxW, UINT maxH)
{
    GameModeList = list;
    ModeListAdapter = adapter;
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

int32_t FindModeIndex(UINT width, UINT height)
{
    int32_t best = -1;
    UINT bestRefreshRate = 0;
    for (uint32_t i = 0; GameModeList && i < GameModeList->count; ++i)
    {
        auto mode = GetListMode(i);
        if (mode && mode->Width == width && mode->Height == height && mode->RefreshRate >= bestRefreshRate)
        {
            best = static_cast<int32_t>(i);
            bestRefreshRate = mode->RefreshRate;
        }
    }
    return best;
}

// The saved resolution is known when the game applies it. Keep it selectable even if the adapter reports too many modes.
int32_t ResolveModeIndex(int32_t width, int32_t height, int32_t index)
{
    const auto w = static_cast<UINT>(width), h = static_cast<UINT>(height);
    if (auto mode = GetListMode(index); mode && mode->Width == w && mode->Height == h)
        return index;
    auto best = FindModeIndex(w, h);
    if (best < 0 && GameModeList && ModeListAdapter < AvailableModes.size())
    {
        const auto& available = AvailableModes[ModeListAdapter];
        if (std::any_of(available.begin(), available.end(), [&](const auto& mode) { return mode.Width == w && mode.Height == h; }))
        {
            const auto previousList = *GameModeList;
            auto previousModes = AdapterModes[ModeListAdapter];
            SetAdapterModes(ModeListAdapter, BuildModeList(available, w, h));
            if (SUCCEEDED(shFillModeList.unsafe_thiscall<int>(GameModeList, ModeListAdapter, 0, 0, INT_MAX, INT_MAX)))
                best = FindModeIndex(w, h);
            if (best < 0)
            {
                SetAdapterModes(ModeListAdapter, std::move(previousModes));
                *GameModeList = previousList;
            }
        }
    }
    return best >= 0 ? best : index;
}

// The new mode gets its size from the arguments, but its refresh rate from the mode at the index
SafetyHookInline shSetMode = {};
void* lastModeContext = nullptr;
static bool committingMode = false;
void* __fastcall SetMode(void* _this, void* edx, int32_t width, int32_t height, int32_t index)
{
    if (!committingMode)
        DPLWindow::pendingMode = {};
    lastModeContext = _this;
    if (!DPLWindow::Dimensions(width, height)) return nullptr;
    index = ResolveModeIndex(width, height, index);
    auto* native = static_cast<int32_t*>(_this);
    if (native[65814] != width || native[65815] != height)
        native[65829] = -1;
    return shSetMode.unsafe_thiscall<void*>(_this, width, height, index);
}

class Resolution
{
public:
    Resolution()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            bForceMaxRefreshRate = settings.Int("MAIN.ForceMaxRefreshRate") != 0;
            nMinResX = settings.Int("MAIN.MinResX");
            nMinResY = settings.Int("MAIN.MinResY");
            nMaxResX = settings.Int("MAIN.MaxResX");
            nMaxResY = settings.Int("MAIN.MaxResY");

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
            // SetMode's native queue target owns the reset flags and the device used below.
            auto renderer = hook::pattern("8D 45 F0 50 53 57 B9 ? ? ? ? E8 ? ? ? ? 83 C6 18");
            static auto* nativeRenderer = *renderer.get_first<uint8_t*>(7);
            static auto& resetPending = nativeRenderer[0];
            static auto& deviceLost = nativeRenderer[1];
            static auto& rendererDevice = *reinterpret_cast<IDirect3DDevice9**>(nativeRenderer + 0x164);
            static int pendingIndex = -1;
            DPLWindow::queueMode = [](DPLWindow::ModeChange& next)
            {
                if (!lastModeContext || resetPending || deviceLost || next.device != rendererDevice)
                    return false;
                auto* native = static_cast<int32_t*>(lastModeContext);
                if (next.width <= 0 || next.height <= 0)
                {
                    next.width = native[65814];
                    next.height = native[65815];
                }
                if (!DPLWindow::Dimensions(next.target, next.width, next.height)) return false;
                pendingIndex = ResolveModeIndex(next.width, next.height, native[65829]);
                if (next.target == 0)
                {
                    const auto* selected = GetListMode(pendingIndex);
                    if (!selected || !selected->RefreshRate)
                    {
                        int width = 0, height = 0;
                        if (!DPLWindow::Dimensions(2, width, height)) return false;
                        pendingIndex = ResolveModeIndex(width, height, -1);
                        selected = GetListMode(pendingIndex);
                        if (!selected || !selected->RefreshRate) return false;
                    }
                    // A resized client can have dimensions that the monitor cannot use exclusively.
                    next.width = selected->Width;
                    next.height = selected->Height;
                    next.refresh = selected->RefreshRate;
                }
                // The reset loop consumes this flag. Keep the previous native parameters and
                // viewports until the real Reset accepts the parameters in its local copy.
                resetPending = 1;
                return true;
            };
            DPLWindow::commitMode = [](const DPLWindow::ModeChange& applied)
            {
                auto* native = static_cast<int32_t*>(lastModeContext);
                native[65829] = -1;
                dwWindowedMode = applied.target != 0;
                committingMode = true;
                SetMode(lastModeContext, nullptr, applied.width, applied.height, pendingIndex);
                committingMode = false;
            };

            // Only the owning renderer's real IDirect3DDevice9::Reset resolves the request.
            auto reset = hook::pattern("8B 08 50 FF 51 40 3D 68 08 76 88 5F 5E");
            static SafetyHookMid BeginModeReset = safetyhook::create_mid(reset.get_first(), [](SafetyHookContext& regs)
            {
                const auto& pending = DPLWindow::pendingMode;
                if (reinterpret_cast<uint8_t*>(regs.ebx) == nativeRenderer &&
                    pending.device && pending.device == rendererDevice && pending.window == DPLWindow::Window())
                    DPLWindow::StageModeReset(reinterpret_cast<D3DPRESENT_PARAMETERS*>(regs.ebp - 0x38));
            });
            static SafetyHookMid FinishModeReset = safetyhook::create_mid(reset.get_first(6), [](SafetyHookContext& regs)
            {
                if (DPLWindow::stagedReset.load() == reinterpret_cast<D3DPRESENT_PARAMETERS*>(regs.ebp - 0x38))
                    DPLWindow::FinishModeReset(static_cast<HRESULT>(regs.eax));
            });

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

            pattern = hook::pattern("89 8E 8C 01 00 00 53 FF 75 08 8B CE E8 ? ? ? ? 8B CE E8");
            static SafetyHookMid ModeDimensionsHook;
            ModeDimensionsHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                int width = *reinterpret_cast<const int*>(regs.ebp - 0x10);
                int height = *reinterpret_cast<const int*>(regs.ebp - 0x0C);
                if (DPLWindow::mode != 0 && DPLWindow::Dimensions(width, height))
                {
                    BackbufferWidth = width;
                    BackbufferHeight = height;
                }
                onResChange().executeAll(*BackbufferWidth, *BackbufferHeight);
            });
            auto apply = hook::pattern("80 3D ? ? ? ? 00 75 15 FF 36 8B 0D ? ? ? ? FF 76 48 8B 01 FF 37 FF 90 98 00 00 00 B0 01");
            // Native Video skips its validated resolution request when the windowed flag is set.
            injector::MakeNOP(apply.get_first(7), 2, true);


            onResChange() += [](int Width, int Height)
            {
                if (DPLWindow::Place(Width, Height))
                    return;
                auto [DesktopResW, DesktopResH] = GetDesktopRes();
                SetWindowPos(hWnd, NULL, (DesktopResW - Width) / 2, (DesktopResH - Height) / 2, Width, Height, SWP_NOACTIVATE | SWP_NOZORDER);
            };
        };
    }
} Resolution;
