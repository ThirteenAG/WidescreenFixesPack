module;

#include <stdafx.h>
#include <d3d9.h>

export module Resolution;

import ComVars;

int32_t nMinResX = 0;
int32_t nMinResY = 0;

std::map<std::pair<int, int>, int> MaxRefreshRateMap;

std::filesystem::path GetResolutionConfigPath()
{
    return GetExeModulePath() / "Saves" / "RES.cfg";
}

struct ResolutionConfig
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t refreshrate = 0;
    D3DFORMAT format = D3DFMT_UNKNOWN;
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
        auto parse = [&](const char* key, auto& out)
        {
            using T = std::remove_reference_t<decltype(out)>;
            if (line.rfind(key, 0) == 0)
            {
                auto pos = line.find('=');
                if (pos != std::string::npos)
                {
                    if constexpr (std::is_same_v<T, uint32_t>)
                        out = static_cast<T>(std::stoul(line.substr(pos + 1)));
                    else if constexpr (std::is_same_v<T, D3DFORMAT>)
                        out = static_cast<T>(std::stoul(line.substr(pos + 1)));
                }
            }
        };

        parse("width", cfg.width);
        parse("height", cfg.height);
        parse("refreshrate", cfg.refreshrate);
        parse("format", cfg.format);
    }

    return cfg;
}

void SaveResolution(uint32_t width, uint32_t height, uint32_t refreshrate, D3DFORMAT format)
{
    auto path = GetResolutionConfigPath();

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open())
        return;

    file << "width = " << width << '\n'
        << "height = " << height << '\n'
        << "refreshrate = " << refreshrate << '\n'
        << "format = " << static_cast<uint32_t>(format) << '\n';
}

IDirect3D9* CreateD3D9()
{
    static IDirect3D9* (WINAPI * pDirect3DCreate9)(UINT) = nullptr;

    if (!pDirect3DCreate9)
    {
        HMODULE hD3D9 = LoadLibraryA("d3d9.dll");
        if (hD3D9)
        {
            pDirect3DCreate9 = reinterpret_cast<decltype(pDirect3DCreate9)>(GetProcAddress(hD3D9, "Direct3DCreate9"));
        }
    }

    if (pDirect3DCreate9)
        return pDirect3DCreate9(D3D_SDK_VERSION);

    return nullptr;
}

D3DFORMAT GetBestFormat(uint32_t width, uint32_t height)
{
    static std::map<std::pair<uint32_t, uint32_t>, D3DFORMAT> formatCache;
    static bool initialized = false;

    if (!initialized)
    {
        initialized = true;

        IDirect3D9* pD3D = CreateD3D9();
        if (pD3D)
        {
            const D3DFORMAT candidates[] = {
                D3DFMT_A2R10G10B10,
                D3DFMT_X8R8G8B8,
                D3DFMT_A8R8G8B8,
                D3DFMT_R5G6B5,
                D3DFMT_X1R5G5B5,
            };

            for (D3DFORMAT fmt : candidates)
            {
                UINT modeCount = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, fmt);
                for (UINT i = 0; i < modeCount; ++i)
                {
                    D3DDISPLAYMODE mode{};
                    if (SUCCEEDED(pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, fmt, i, &mode)))
                    {
                        auto key = std::make_pair(mode.Width, mode.Height);
                        if (formatCache.find(key) == formatCache.end())
                        {
                            formatCache[key] = fmt;
                        }
                    }
                }
            }

            pD3D->Release();
        }
    }

    auto key = std::make_pair(width, height);
    auto it = formatCache.find(key);

    if (it != formatCache.end())
        return it->second;

    return D3DFMT_X8R8G8B8;
}

class Resolution
{
public:
    Resolution()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            nMinResX = iniReader.ReadInteger("MAIN", "MinResX", 0);
            nMinResY = iniReader.ReadInteger("MAIN", "MinResY", 0);

            auto ResList = GetResolutionsList(true);
            for (const auto& entry : ResList)
            {
                auto key = std::make_pair(std::get<0>(entry), std::get<1>(entry));
                int refresh = std::get<2>(entry);
                auto it = MaxRefreshRateMap.find(key);
                if (it == MaxRefreshRateMap.end() || refresh > it->second)
                    MaxRefreshRateMap[key] = refresh;
            }

            if (!nMinResX || !nMinResY)
            {
                auto ResList = GetResolutionsList(false);
                if (ResList.size() > 100)
                    ResList.erase(ResList.begin(), ResList.begin() + (ResList.size() - 100));

                nMinResX = std::get<0>(ResList.front());
                nMinResY = std::get<1>(ResList.front());
            }

            //uncap resolutions
            auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 57 8B DE E8 ? ? ? ? 85 C0"); //54A5A0
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), nMinResY, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), nMinResX, true);

            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 85"); //57F665
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), nMinResY, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), nMinResX, true);

            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 50 33 DB E8"); //5C9288
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), nMinResY, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), nMinResX, true);

            //default to desktop res
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            pattern = hook::pattern("EB ? C7 04 24 ? ? ? ? C7 44 24 04"); //54A7C7
            injector::MakeNOP(pattern.get_first(0), 2, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(5 + 0), DesktopResW, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(5 + 8), DesktopResH, true);
            pattern = hook::pattern("56 57 56 56 68 ? ? ? ? 68 ? ? ? ? 56 56"); //576C6B
            injector::WriteMemory(pattern.get_first<int32_t*>(5), DesktopResH, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(10), DesktopResW, true);

            //1024x768 black screen fix
            pattern = hook::pattern("C7 44 24 28 ? ? ? ? C7 44 24 2C ? ? ? ? C7 44 24 30 ? ? ? ? C7 44 24 34 ? ? ? ? 89 16 E8"); //0x57F6DF
            injector::WriteMemory(pattern.get_first<int32_t*>(4 + 0), DesktopResW, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(4 + 8), DesktopResH, true);

            //force 32 bit HD and max refresh rate
            pattern = hook::pattern("8B 02 3B 44 24 ? 8B 4A ? 8B 72 ? 8B 52 ? 89 74 24");
            injector::MakeNOP(pattern.get_first(0), 2, true);
            static auto ResHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto w = *(uint32_t*)(regs.edx + 0x0);
                auto h = *(uint32_t*)(regs.edx + 0x4);
                auto refreshrate = *(uint32_t*)(regs.edx + 0x8);
                auto format = *(D3DFORMAT*)(regs.edx + 0xC);

                regs.eax = w;

                auto key = std::make_pair(w, h);
                auto it = MaxRefreshRateMap.find(key);
                if (it != MaxRefreshRateMap.end() && refreshrate != it->second)
                    regs.eax = 0;

                if (format != GetBestFormat(w, h))
                    regs.eax = 0;
            });

            //main menu resolution (before profile loading)
            pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? ? ? E8");
            struct DefaultResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto [DesktopResW, DesktopResH] = GetDesktopRes();
                    auto config = GetSavedResolution();
                    auto w = config.width ? config.width : DesktopResW;
                    auto h = config.height ? config.height : DesktopResH;
                    auto refreshrate = config.refreshrate ? config.refreshrate : MaxRefreshRateMap[std::make_pair(DesktopResW, DesktopResH)];
                    auto format = config.format != D3DFMT_UNKNOWN ? config.format : GetBestFormat(w, h);

                    *(uint32_t*)(regs.esp + 0x28) = w;
                    *(uint32_t*)(regs.esp + 0x2C) = h;
                    *(uint32_t*)(regs.esp + 0x30) = refreshrate;
                    *(D3DFORMAT*)(regs.esp + 0x34) = format;
                }
            }; injector::MakeInline<DefaultResHook>(pattern.get_first(0), pattern.get_first(32));

            pattern = hook::pattern("C7 04 24 ? ? ? ? C7 44 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? C7 44 24 ? ? ? ? ? 56 8D 74 24 ? 8B CF");
            struct DefaultResHook2
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto [DesktopResW, DesktopResH] = GetDesktopRes();
                    auto config = GetSavedResolution();
                    auto w = config.width ? config.width : DesktopResW;
                    auto h = config.height ? config.height : DesktopResH;
                    *(uint32_t*)(regs.esp + 0x0) = w;
                    *(uint32_t*)(regs.esp + 0x4) = h;
                }
            }; injector::MakeInline<DefaultResHook2>(pattern.get_first(0), pattern.get_first(15));

            pattern = hook::pattern("C7 04 24 ? ? ? ? C7 44 24 ? ? ? ? ? 56 8D 74 24 ? 8B CF C7 44 24 ? ? ? ? ? C7 44 24");
            struct DefaultResHook3
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto [DesktopResW, DesktopResH] = GetDesktopRes();
                    auto config = GetSavedResolution();
                    auto w = config.width ? config.width : DesktopResW;
                    auto h = config.height ? config.height : DesktopResH;
                    *(uint32_t*)(regs.esp + 0x0) = w;
                    *(uint32_t*)(regs.esp + 0x4) = h;
                }
            }; injector::MakeInline<DefaultResHook3>(pattern.get_first(0), pattern.get_first(15));

            pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 85 C0 7D");
            struct DefaultResHook4
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto [DesktopResW, DesktopResH] = GetDesktopRes();
                    auto config = GetSavedResolution();
                    auto refreshrate = config.refreshrate ? config.refreshrate : MaxRefreshRateMap[std::make_pair(DesktopResW, DesktopResH)];
                    auto format = config.format != D3DFMT_UNKNOWN ? config.format : GetBestFormat(DesktopResW, DesktopResH);

                    *(uint32_t*)(regs.esp + 0x0C) = refreshrate;
                    *(D3DFORMAT*)(regs.esp + 0x10) = format;
                }
            }; injector::MakeInline<DefaultResHook4>(pattern.get_first(0), pattern.get_first(16));

            //resolution switch
            pattern = hook::pattern("89 8B 80 01 00 00"); //0x5E4C73
            struct GetResHook1
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ebx + 0x180) = regs.ecx;

                    auto w = *(uint32_t*)(regs.ebx + 0x160);
                    auto h = *(uint32_t*)(regs.ebx + 0x164);
                    auto refreshrate = *(uint32_t*)(regs.ebx + 0x190);
                    auto format = *(D3DFORMAT*)(regs.ebx + 0x168);

                    SaveResolution(w, h, refreshrate, format);
                    onResChange().executeAll(w, h);
                }
            };
            struct GetResHook2
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ebx + 0x180) = regs.ecx;

                    auto& w = *(uint32_t*)(regs.ebx + 0x160);
                    auto& h = *(uint32_t*)(regs.ebx + 0x164);
                    auto& refreshrate = *(uint32_t*)(regs.ebx + 0x190);
                    auto& format = *(D3DFORMAT*)(regs.ebx + 0x168);

                    static bool bOnce = false;
                    if (!bOnce)
                    {
                        bOnce = true;

                        auto [DesktopResW, DesktopResH] = GetDesktopRes();
                        auto config = GetSavedResolution();
                        w = config.width ? config.width : DesktopResW;
                        h = config.height ? config.height : DesktopResH;
                        refreshrate = config.refreshrate ? config.refreshrate : MaxRefreshRateMap[std::make_pair(DesktopResW, DesktopResH)];
                        format = config.format != D3DFMT_UNKNOWN ? config.format : GetBestFormat(w, h);
                    }
                    else
                        SaveResolution(w, h, refreshrate, format);
                    onResChange().executeAll(w, h);
                }
            };
            injector::MakeInline<GetResHook1>(pattern.count(2).get(0).get<void*>(0), pattern.count(2).get(0).get<void*>(6));
            injector::MakeInline<GetResHook2>(pattern.count(2).get(1).get<void*>(0), pattern.count(2).get(1).get<void*>(6));

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