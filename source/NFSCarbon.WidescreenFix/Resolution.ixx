module;

#include <stdafx.h>

export module Resolution;

import ComVars;

using FEString = void;

struct Res
{
    int width;
    int height;
    int refreshRate;
};

std::vector<Res> ResList;
std::unordered_map<int, std::string> ResListText;
std::vector<int> ResListHashes;
std::vector<int> dwSupportedResolutionsArray;
std::vector<int> dwSupportedRefreshRateArray;
int cachedWidth, cachedHeight;

namespace DALOptions
{
    SafetyHookInline shIncScreenRes = {};
    char __stdcall IncScreenRes(int i)
    {
        int res = g_RacingResolution;

        do
        {
            res += i;

            if (res >= (int)ResList.size())
                res = 0;
            else if (res < 0)
                res = (int)ResList.size() - 1;

        } while (!dwSupportedResolutionsArray[res]);

        g_RacingResolution = res;
        return 1;
    }
}

void __stdcall GetResolution(int* outWidth, int* outHeight)
{
    if (!outWidth || !outHeight)
        return;

    if (g_RacingResolution < 0 || g_RacingResolution >= (int)ResList.size())
    {
        auto desktopRes = GetDesktopRes();
        *outWidth = std::get<0>(desktopRes);
        *outHeight = std::get<1>(desktopRes);
    }
    else
    {
        *outWidth = ResList[g_RacingResolution].width;
        *outHeight = ResList[g_RacingResolution].height;
    }
}

SafetyHookInline shsub_712AC0 = {};
void __stdcall GetRacingResolution(int* outWidth, int* outHeight)
{
    GetResolution(outWidth, outHeight);
    cachedWidth = *outWidth;
    cachedHeight = *outHeight;
    onResChange().executeAll(*outWidth, *outHeight);
}

SafetyHookInline shsub_712BA0 = {};
void __stdcall sub_712BA0(int* outWidth, int* outHeight)
{
    return GetRacingResolution(outWidth, outHeight);
}

SafetyHookInline shGetLocalizedString = {};
char* __cdecl GetLocalizedString(int hash)
{
    return shGetLocalizedString.unsafe_ccall<char*>(hash);
}

SafetyHookInline shGetLocalizedWideString = {};
char __cdecl GetLocalizedWideString(wchar_t* out, int size, int hash)
{
    auto ret = shGetLocalizedWideString.unsafe_ccall<char>(out, size, hash);

    if (!ret)
    {
        if (auto it = ResListText.find(hash); it != ResListText.end())
        {
            std::wstring ws(it->second.begin(), it->second.end());
            wcscpy_s(out, std::min(size, int(ws.size()) + 1), ws.c_str());
            return 1;
        }
    }

    return ret;
}

namespace cFEngRender
{
    SafetyHookInline shGetStringToRender = {};
    void __cdecl GetStringToRender(wchar_t* out, FEString* str, int size)
    {
        auto hash = *(uint32_t*)((uintptr_t)str + 0x18);
        shGetStringToRender.unsafe_ccall(out, str, size);
    }
}

namespace DALLangHash
{
    SafetyHookInline shGetLocg_RacingResolution = {};
    char __stdcall GetLocg_RacingResolution(int* outHash, int i)
    {
        if (g_RacingResolution < 0 || g_RacingResolution >= (int)ResList.size())
            return 0;

        *outHash = ResListHashes[i];
        return 1;
    }
}

SafetyHookInline shsub_72B370 = {};
void __cdecl sub_72B370()
{
    shsub_72B370.unsafe_ccall();

    if (dwWindowedMode)
    {
        int Width, Height;
        GetRacingResolution(&Width, &Height);

        auto [deskW, deskH] = GetDesktopRes();
        int windowW = Width;
        int windowH = Height;
        int left = (deskW - windowW) / 2;
        int top = (deskH - windowH) / 2;

        WindowRect->left = left;
        WindowRect->top = top;
        WindowRect->right = left + windowW;
        WindowRect->bottom = top + windowH;

        SetWindowPos(hWnd, HWND_NOTOPMOST, WindowRect->left, WindowRect->top, WindowRect->right - WindowRect->left, WindowRect->bottom - WindowRect->top, SWP_SHOWWINDOW);
    }
}

export std::pair<int, int> GetRes()
{
    return { cachedWidth, cachedHeight };
}

export float GetAspectRatio()
{
    auto [Width, Height] = GetRes();
    return static_cast<float>(Width) / static_cast<float>(Height);
}

class Resolution
{
public:
    Resolution()
    {
        WFP::onInitEvent() += []()
        {
            for (const auto& res : GetResolutionsList(false))
            {
                auto [width, height, refresh] = res;
                ResList.push_back({ width, height, refresh });
                auto label = std::format("{}x{}", width, height);
                auto hash = bStringHash(std::format("OPTIONS_PC_RES_{}X{}", width, height).c_str());
                ResListText[hash] = label;
                ResListHashes.push_back(hash);
                dwSupportedResolutionsArray.push_back(1);
                dwSupportedRefreshRateArray.push_back(refresh);
            }

            auto pattern = hook::pattern("89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? 89 2D ? ? ? ? ? ? 50");
            auto pSupportedResolutionsArray = *pattern.get_first<void*>(2);
            auto pSupportedRefreshRateArray = *pattern.get_first<void*>(8);

            pattern = hook::pattern("8B 14 85 ? ? ? ? 85 D2 74 ? A3");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 75 ? 85 C0");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 75 ? 48");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 8B 54 24 ? ? ? C2");
            injector::AdjustPointer(pattern.count_hint(2).get(0).get<void*>(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);
            injector::AdjustPointer(pattern.count_hint(2).get(1).get<void*>(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 14 8D ? ? ? ? 8B 44 24 ? 8B 4C 24 ? 52 50");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 14 BD ? ? ? ? 8B 44 24");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 04 95 ? ? ? ? ? ? ? 8B 54 24");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 14 8D ? ? ? ? 8B 0D");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("3B B3 ? ? ? ? C7 83");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("89 B3 ? ? ? ? 3B 3D");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 2C 95 ? ? ? ? E8 ? ? ? ? 8B 7C 24 ? 8B 5C 24 ? B8");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 04 95 ? ? ? ? 8B 4C 24");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("A1 ? ? ? ? 8B 4C 24 ? 8D A4 24");
            DALOptions::shIncScreenRes = safetyhook::create_inline(pattern.get_first(), DALOptions::IncScreenRes);

            pattern = hook::pattern("89 1D ? ? ? ? 57 56");
            injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("89 35 ? ? ? ? 83 F8");
            injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("A1 ? ? ? ? 83 F8 ? 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 8B 44 24");
            shsub_712AC0 = safetyhook::create_inline(pattern.count_hint(2).get(0).get<void*>(), GetRacingResolution);
            shsub_712BA0 = safetyhook::create_inline(pattern.count_hint(2).get(1).get<void*>(), sub_712BA0);

            pattern = hook::pattern("E8 ? ? ? ? 8B 4C 24 ? 50 8B 44 24 ? 50 51 E8 ? ? ? ? 83 C4 ? B0 ? 5E");
            shGetLocalizedString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), GetLocalizedString);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 84 C0 74 ? 8D 54 24 ? 89 54 24");
            shGetLocalizedWideString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), GetLocalizedWideString);

            pattern = hook::pattern("E8 ? ? ? ? 57 E8 ? ? ? ? 83 C4 ? 85 C0");
            cFEngRender::shGetStringToRender = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), cFEngRender::GetStringToRender);

            pattern = hook::pattern("E8 ? ? ? ? 5F 5E 5B C2 ? ? 8B 4C 24 ? 57 51 8B CE E8 ? ? ? ? 5F 5E 5B C2 ? ? 8B 54 24 ? 57 52 8B CE E8 ? ? ? ? 5F 5E 5B C2 ? ? 8B 44 24 ? 57 50 8B CE E8 ? ? ? ? 5F 5E 5B C2 ? ? 5F");
            DALLangHash::shGetLocg_RacingResolution = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), DALLangHash::GetLocg_RacingResolution);

            pattern = hook::pattern("6A ? 68 ? ? ? ? 6A ? 6A ? 68 ? ? ? ? 52 FF D6");
            static auto RacingResolutionSet = safetyhook::create_mid(pattern.count_hint(6).get(3).get<void*>(7), [](SafetyHookContext& regs)
            {
                int index = g_RacingResolution;
                if (index >= 0 && index < (int)ResList.size())
                {
                    static int packed = 0;
                    int width = ResList[index].width;
                    int height = ResList[index].height;
                    // Pack width and height as shorts into an int, and set sign bit as flag: (width << 16) | height | 0x80000000
                    packed = (width << 16) | (height & 0xFFFF) | 0x80000000;
                    *(int**)(regs.esp + 0) = &packed;
                }
            });

            pattern = hook::pattern("8B 44 24 ? 8D 4C 24 ? 51 68 ? ? ? ? 8D 54 24 ? 52 6A ? 68 ? ? ? ? 50 89 7C 24 ? 89 7C 24 ? FF D6 89 7C 24");
            static auto RacingResolutionGet = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                int packed = g_RacingResolution;
                int index = -1;
                if (packed & 0x80000000) // Sign bit set, it's packed
                {
                    int width = (packed & 0x7FFF0000) >> 16;
                    int height = packed & 0xFFFF;
                    for (size_t i = 0; i < ResList.size(); ++i)
                    {
                        if (ResList[i].width == width && ResList[i].height == height)
                        {
                            index = (int)i;
                            break;
                        }
                    }
                }
                // If not packed or not found, default to desktop resolution index
                if (index == -1)
                {
                    auto [deskW, deskH] = GetDesktopRes();
                    for (size_t i = 0; i < ResList.size(); ++i)
                    {
                        if (ResList[i].width == deskW && ResList[i].height == deskH)
                        {
                            index = (int)i;
                            break;
                        }
                    }
                    if (index == -1) index = 0; // Fallback to 0 if desktop resolution not in list
                }
                g_RacingResolution = index;
            });

            pattern = hook::pattern("E8 ? ? ? ? 88 1D ? ? ? ? 55");
            shsub_72B370 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), sub_72B370);
        };
    }
} Resolution;