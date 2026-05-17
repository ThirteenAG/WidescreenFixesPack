module;

#include <stdafx.h>
#include <d3d9.h>

export module Resolution;

import ComVars;

int32_t nMinResX = 0;
int32_t nMinResY = 0;

std::map<std::pair<int, int>, int> MaxRefreshRateMap;

int nCurrentResX = 0;
int nCurrentResY = 0;
int nCurrentRefresh = 0;

SafetyHookInline shsub_5E3DDC = {};
int __fastcall sub_5E3DDC(void* _this, void* edx, UINT Adapter, unsigned int a3, unsigned int a4, unsigned int a5, unsigned int a6)
{
    return shsub_5E3DDC.unsafe_fastcall<int>(_this, edx, Adapter, nMinResX, nMinResY, INT_MAX, INT_MAX);
}

char __fastcall sub_5E3C9D(D3DFORMAT fmt, void*)
{
    auto key = std::make_pair(nCurrentResX, nCurrentResY);
    auto it = MaxRefreshRateMap.find(key);
    if (it != MaxRefreshRateMap.end() && nCurrentRefresh != it->second)
        return 0;

    if (fmt == D3DFMT_A2R10G10B10)
        return 1;

    return 0;
}

injector::hook_back<int(__fastcall*)(void*, void*, int*, int*, int*)> hbsub_5E3A10;
int __fastcall sub_5E3A10(void* _this, void* edx, int* a2, int* a3, int* a4)
{
    auto ret = hbsub_5E3A10.fun(_this, edx, a2, a3, a4);

    nCurrentResX = a4[0];
    nCurrentResY = a4[1];
    nCurrentRefresh = a4[2];

    return ret;
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

            //cache processed resolutions
            pattern = hook::pattern("E8 ? ? ? ? 3B C3 0F 8C ? ? ? ? 8B 4D");
            hbsub_5E3A10.fun = injector::MakeCALL(pattern.get_first(), sub_5E3A10, true).get();

            //force 32 bit HD and max refresh rate
            pattern = hook::pattern("E8 ? ? ? ? 84 C0 74 ? 8B 45 ? 3B 45");
            injector::MakeCALL(pattern.get_first(), sub_5E3C9D, true);

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