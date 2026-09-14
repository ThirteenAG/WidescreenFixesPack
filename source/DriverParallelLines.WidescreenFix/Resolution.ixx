module;

#include <stdafx.h>
#include <d3d9.h>

export module Resolution;

import ComVars;

bool* bIsWindowed = nullptr;

int32_t nMinResX = 0;
int32_t nMinResY = 0;

SafetyHookInline shsub_5E3DDC = {};
int __fastcall sub_5E3DDC(void* _this, void* edx, UINT Adapter, unsigned int a3, unsigned int a4, unsigned int a5, unsigned int a6)
{
    return shsub_5E3DDC.unsafe_fastcall<int>(_this, edx, Adapter, nMinResX, nMinResY, INT_MAX, INT_MAX);
}

std::map<std::pair<int, int>, int> MaxRefreshRateMap;

int nCurrentResX = 0;
int nCurrentResY = 0;
int nCurrentRefresh = 0;
int nCurrentFormat = 0;

injector::hook_back<void(__fastcall*)(void*, void*, int*)> hbsub_5E76A9;
void __fastcall sub_5E76A9(void* _this, void* edx, int* a2)
{
    nCurrentResX = a2[0];      // width
    nCurrentResY = a2[1];      // height
    nCurrentRefresh = a2[2];   // refresh rate
    nCurrentFormat = a2[3];    // D3DFORMAT (21=A8R8G8B8, 22=X8R8G8B8, 35=A2R10G10B10)

    auto is32bit = [](D3DFORMAT f)
    {
        return f == D3DFMT_A8R8G8B8 || f == D3DFMT_X8R8G8B8 || f == D3DFMT_A2R10G10B10;
    };

    auto key = std::make_pair(nCurrentResX, nCurrentResY);
    auto it = MaxRefreshRateMap.find(key);
    if (it != MaxRefreshRateMap.end() && nCurrentRefresh > it->second)
        it->second = nCurrentRefresh;

    static std::map<std::pair<int, int>, D3DFORMAT> appended;

    auto prev = appended.find(key);
    if (prev != appended.end())
    {
        bool prev32 = is32bit(prev->second);
        bool cur32 = is32bit((D3DFORMAT)nCurrentFormat);

        if (prev32 && !cur32) return;
        if (prev32 && cur32)  return;
    }

    appended[key] = (D3DFORMAT)nCurrentFormat;
    hbsub_5E76A9.fun(_this, edx, a2);
}

class Resolution
{
public:
    Resolution()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto ForceMaxRefreshRate = iniReader.ReadInteger("MAIN", "ForceMaxRefreshRate", 0);

            auto ResList = GetResolutionsList(true);
            for (const auto& entry : ResList)
            {
                auto key = std::make_pair(std::get<0>(entry), std::get<1>(entry));
                int refresh = std::get<2>(entry);
                auto it = MaxRefreshRateMap.find(key);
                if (it == MaxRefreshRateMap.end() || refresh > it->second)
                    MaxRefreshRateMap[key] = refresh;
            }

            if (ResList.size() > 100)
                ResList.erase(ResList.begin(), ResList.begin() + 100);

            //force 32 bit HD and max refresh rate
            if (ForceMaxRefreshRate)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 43 3B 5D ? 72 ? 47");
                hbsub_5E76A9.fun = injector::MakeCALL(pattern.get_first(), sub_5E76A9, true).get();

                nMinResX = 0;
                nMinResY = 0;
            }
            else
            {
                nMinResX = std::get<0>(ResList.front());
                nMinResY = std::get<1>(ResList.front());
            }

            //uncap resolutions
            auto pattern = hook::pattern("E8 ? ? ? ? 85 C0 0F 8C ? ? ? ? 8D 45");
            shsub_5E3DDC = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), sub_5E3DDC);

            //default to desktop res
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            pattern = hook::pattern("C7 46 4C ? ? ? ? C7 46 50 ? ? ? ? C7 46 54 ? ? ? ? C7"); //4C5077
            injector::WriteMemory(pattern.get_first<int32_t*>(3 + 0), DesktopResW, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(3 + 7), DesktopResH, true);

            //1024x768 black screen fix
            pattern = hook::pattern("C7 45 E0 ? ? ? ? C7 45 E4 ? ? ? ? C7 45 E8 ? ? ? ? C7 45 EC 16 00 00 00"); //0x5EBA06
            injector::WriteMemory(pattern.get_first<int32_t*>(3 + 0), DesktopResW, true);
            injector::WriteMemory(pattern.get_first<int32_t*>(3 + 7), DesktopResH, true);

            pattern = hook::pattern("80 3D ? ? ? ? ? 75 ? ? ? 8B 0D");
            bIsWindowed = *pattern.get_first<decltype(bIsWindowed)>(2);

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