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
    void __fastcall IncScreenRes(void* _this, void* edx, int a2, int actionId)
    {
        int* pCurrentIndex = reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(*pOptions) + 108);
        int currentIndex = *pCurrentIndex;

        constexpr int kDecrementAction = 0x9120409E;
        constexpr int kIncrementAction = 0xB5971BF1;

        int delta = 0;
        bool shouldChangeIndex = false;

        if (actionId == kDecrementAction)
        {
            delta = -1;
            shouldChangeIndex = true;
        }
        else if (actionId == kIncrementAction)
        {
            delta = 1;
            shouldChangeIndex = true;
        }

        if (shouldChangeIndex)
        {
            int kNumResolutions = (int)ResList.size();

            do
            {
                currentIndex += delta;

                if (currentIndex < 0)
                    currentIndex = kNumResolutions - 1;
                else if (currentIndex >= kNumResolutions)
                    currentIndex = 0;
            } while (!dwSupportedResolutionsArray[currentIndex]);

            *pCurrentIndex = currentIndex;
        }

        reinterpret_cast<BYTE*>(_this)[42] = 1;
        DWORD* vtable = *reinterpret_cast<DWORD**>(_this);

        using NotifyFn = void (__thiscall*)(void* _this, int actionId);
        NotifyFn notifyChanged = reinterpret_cast<NotifyFn>(vtable[14]);
        notifyChanged(_this, actionId);

        using UpdateFn = int (__thiscall*)(void* _this);
        UpdateFn update = reinterpret_cast<UpdateFn>(vtable[3]);
        update(_this);
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

SafetyHookInline shsub_6C27D0 = {};
void __stdcall GetRacingResolution(int* outWidth, int* outHeight)
{
    GetResolution(outWidth, outHeight);
    cachedWidth = *outWidth;
    cachedHeight = *outHeight;
    onResChange().executeAll(*outWidth, *outHeight);
}

SafetyHookInline shsub_6C28B0 = {};
void __stdcall sub_6C28B0(int* outWidth, int* outHeight)
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

SafetyHookInline shsub_6DB0D0 = {};
void __cdecl sub_6DB0D0()
{
    shsub_6DB0D0.unsafe_ccall();

    if (dwWindowedMode)
    {
        int Width, Height;
        GetRacingResolution(&Width, &Height);

        auto [deskW, deskH] = GetDesktopRes();
        int windowW = Width;
        int windowH = Height;
        int left, top;

        LONG style = GetWindowLong(hWnd, GWL_STYLE);
        LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

        if (style & WS_CAPTION)
        {
            // Expand outer size so client rect = Width x Height
            RECT rect = { 0, 0, Width, Height };
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);
            windowW = rect.right - rect.left;
            windowH = rect.bottom - rect.top;

            // Center by client area
            left = (deskW - Width) / 2 + rect.left;
            top = (deskH - Height) / 2 + rect.top;
        }
        else
        {
            left = (deskW - windowW) / 2;
            top = (deskH - windowH) / 2;
        }

        WindowRect->left = left;
        WindowRect->top = top;
        WindowRect->right = left + windowW;
        WindowRect->bottom = top + windowH;

        SetWindowPos(hWnd, HWND_NOTOPMOST, WindowRect->left, WindowRect->top, WindowRect->right - WindowRect->left, WindowRect->bottom - WindowRect->top, SWP_SHOWWINDOW);
    }
}

SafetyHookInline shWndProc = {};
LRESULT __stdcall WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_SYSKEYDOWN:
        {
            if (dwWindowedMode && wParam == VK_MENU)
                return 0;
        }
        break;
        case WM_SETCURSOR:
        {
            if (dwWindowedMode && LOWORD(lParam) != HTCLIENT)
                return DefWindowProc(hWnd, Msg, wParam, lParam);
        }
        break;
        case WM_SIZE:
        {
            cachedWidth = LOWORD(lParam);
            cachedHeight = HIWORD(lParam);
            onResChange().executeAll(cachedWidth, cachedHeight);
        }
        break;
    }

    return shWndProc.unsafe_stdcall<LRESULT>(hWnd, Msg, wParam, lParam);
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
                auto hash = bStringHash(std::format("OPT_VO_PC_RES_{}X{}", width, height).c_str());
                ResListText[hash] = label;
                ResListHashes.push_back(hash);
                dwSupportedResolutionsArray.push_back(1);
                dwSupportedRefreshRateArray.push_back(refresh);
            }

            auto pattern = hook::pattern("89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D");
            auto pSupportedResolutionsArray = *pattern.get_first<void*>(2);
            auto pSupportedRefreshRateArray = *pattern.get_first<void*>(8);

            pattern = hook::pattern("8B 1C 85 ? ? ? ? 85 DB");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 75 ? 85 C0");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 75 ? 48");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 8B 54 24 ? ? ? C2");
            injector::AdjustPointer(pattern.count_hint(2).get(0).get<void*>(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);
            injector::AdjustPointer(pattern.count_hint(2).get(1).get<void*>(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 04 95 ? ? ? ? 8B 4C 24 ? 8B 54 24 ? 50 51 52 E8 ? ? ? ? 83 C4 ? 33 FF");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 0C BD ? ? ? ? 8B 54 24");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 04 95 ? ? ? ? ? ? ? 8B 54 24");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 14 8D ? ? ? ? 8B 0D");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 2C 8D ? ? ? ? 8B 7C 24 ? B8 ? ? ? ? 8D 56 ? 8D 4C 24 ? 89 54 24");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("8B 04 95 ? ? ? ? 8B 4C 24 ? 8B 54 24 ? 50 51 52 E8 ? ? ? ? 83 C4 ? E8");
            injector::AdjustPointer(pattern.get_first(), dwSupportedRefreshRateArray.data(), pSupportedRefreshRateArray, pSupportedRefreshRateArray);

            pattern = hook::pattern("A1 ? ? ? ? 56 57 8B 7C 24 ? 81 FF");
            DALOptions::shIncScreenRes = safetyhook::create_inline(pattern.get_first(), DALOptions::IncScreenRes);

            pattern = hook::pattern("89 3D ? ? ? ? 89 3D ? ? ? ? 89 3D ? ? ? ? 89 35 ? ? ? ? 89 3D ? ? ? ? 89 35");
            injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("89 35 ? ? ? ? 89 35 ? ? ? ? 89 35 ? ? ? ? 89 35 ? ? ? ? 89 35 ? ? ? ? 89 35 ? ? ? ? 89 35");
            injector::MakeNOP(pattern.get_first(), 6, true);


            pattern = hook::pattern("A1 ? ? ? ? 83 F8 ? 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 8B 44 24 ? 8B 4C 24 ? ? ? ? ? ? ? ? ? ? ? ? ? C2");
            shsub_6C27D0 = safetyhook::create_inline(pattern.count_hint(2).get(0).get<void*>(), GetRacingResolution);
            shsub_6C28B0 = safetyhook::create_inline(pattern.count_hint(2).get(1).get<void*>(), sub_6C28B0);


            pattern = hook::pattern("E8 ? ? ? ? 50 6A ? 6A ? 68 ? ? ? ? 6A ? 68 ? ? ? ? 56 E8 ? ? ? ? 83 C4 ? 32 C0 5E C3");
            shGetLocalizedString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), GetLocalizedString);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 84 C0 8D 6C 24 ? 75 ? 8B 6E ? 8B 56");
            shGetLocalizedWideString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), GetLocalizedWideString);

            pattern = hook::pattern("8B 77 ? 85 F6 74 ? 81 4E ? ? ? ? ? 89 46 ? 8B 0D ? ? ? ? 85 C9 74 ? ? ? 56 ? ? 8B 46 ? 83 E0 ? 0D ? ? ? ? 89 46 ? 5F 5E C3");
            static auto GetLocg_RacingResolution = safetyhook::create_mid(pattern.count_hint(4).get(1).get<void*>(0), [](SafetyHookContext& regs)
            {
                int* pCurrentIndex = reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(*pOptions) + 108);
                int currentIndex = *pCurrentIndex;

                if (g_RacingResolution < 0 || g_RacingResolution >= (int)ResList.size())
                    return;

                regs.eax = ResListHashes[currentIndex];
            });

            pattern = hook::pattern("6A ? 68 ? ? ? ? 6A ? 6A ? 68 ? ? ? ? 51 FF D6");
            static auto RacingResolutionSet = safetyhook::create_mid(pattern.count_hint(6).get(4).get<void*>(7), [](SafetyHookContext& regs)
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

            pattern = hook::pattern("8B 44 24 ? 8D 4C 24 ? 51 68 ? ? ? ? 8D 54 24 ? 52 6A ? 68 ? ? ? ? 50 89 7C 24 ? 89 7C 24 ? FF D6 8B 44 24");
            static auto RacingResolutionGet = safetyhook::create_mid(pattern.count_hint(13).get(9).get<void*>(0), [](SafetyHookContext& regs)
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

            pattern = hook::pattern("E8 ? ? ? ? C6 05 ? ? ? ? ? 39 2D ? ? ? ? 89 2D");
            shsub_6DB0D0 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), sub_6DB0D0);

            pattern = hook::pattern("C7 44 24 ? ? ? ? ? 89 5C 24 ? 89 5C 24 ? FF D6 68");
            shWndProc = safetyhook::create_inline(*pattern.get_first<void*>(4), WndProc);
        };
    }
} Resolution;