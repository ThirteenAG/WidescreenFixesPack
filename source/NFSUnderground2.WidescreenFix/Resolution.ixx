module;

#include <stdafx.h>

export module Resolution;

import ComVars;

struct Res
{
    int width;
    int height;
    int refreshRate;
};

std::vector<Res> ResList;
std::vector<std::string> ResListText;
std::vector<int> dwSupportedResolutionsArray;
int cachedWidth, cachedHeight;

namespace DALOptions
{
    uint8_t* byte_8709D1 = nullptr;

    SafetyHookInline shIncScreenRes = {};
    void __fastcall IncScreenRes(void* _this, void* edx, int a2, int actionId)
    {
        const int MAX_RES_INDEX = (int)ResList.size() - 1;

        int oldRes = g_RacingResolution;
        int newRes = oldRes;

        if (actionId == 0xB5971BF1)
        {
            if (oldRes < MAX_RES_INDEX)
            {
                newRes = oldRes + 1;
                while (newRes < MAX_RES_INDEX && !dwSupportedResolutionsArray[newRes])
                    newRes++;
            }
        }
        else if (actionId == 0x9120409E && oldRes > 0)
        {
            newRes = oldRes - 1;
            while (newRes > 0 && !dwSupportedResolutionsArray[newRes])
                newRes--;
        }

        g_RacingResolution = newRes;

        uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(_this);
        using UpdateFn = void (__thiscall*)(void*);
        UpdateFn update = reinterpret_cast<UpdateFn>(vtable[3]);

        if (dwSupportedResolutionsArray[newRes])
        {
            if (newRes != oldRes)
                *byte_8709D1 = 1;

            update(_this);
        }
        else
        {
            g_RacingResolution = oldRes;
            update(_this);
        }
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

SafetyHookInline shsub_5BF610 = {};
void __stdcall GetRacingResolution(int* outWidth, int* outHeight)
{
    GetResolution(outWidth, outHeight);
    cachedWidth = *outWidth;
    cachedHeight = *outHeight;
    onResChange().executeAll(*outWidth, *outHeight);
}

SafetyHookInline shGetLocalizedString = {};
char* __cdecl GetLocalizedString(int hash)
{
    return shGetLocalizedString.unsafe_ccall<char*>(hash);
}

SafetyHookInline shGetLocalizedWideString = {};
char __cdecl GetLocalizedWideString(wchar_t* out, int size, int hash)
{
    return shGetLocalizedWideString.unsafe_ccall<char>(out, size, hash);
}

namespace FEString
{
    void (__fastcall* SetString)(void* FEString, void* edx, const char* str) = nullptr;
}

namespace DALLangHash
{
    void (__cdecl* FEngSetLanguageHash)(void* FEString, int hash) = nullptr;

    SafetyHookInline shGetLocg_RacingResolution = {};
    void __fastcall GetLocg_RacingResolution(void** _this, void* edx)
    {
        FEngSetLanguageHash(_this[11], 0xA9A76BBA);

        if (g_RacingResolution < 0 || g_RacingResolution >= (int)ResList.size())
            return FEString::SetString(_this[12], edx, "undefined");

        FEString::SetString(_this[12], edx, ResListText[g_RacingResolution].c_str());
    }
}

SafetyHookInline shsub_5D1C40 = {};
void __cdecl sub_5D1C40()
{
    shsub_5D1C40.unsafe_ccall();

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
                ResListText.push_back(label);
                dwSupportedResolutionsArray.push_back(1);
            }

            auto pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 75 ? 85 C0");
            auto pSupportedResolutionsArray = *pattern.get_first<void*>(3);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 75 ? 85 C0");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 75 ? 48");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("C6 05 ? ? ? ? ? ? ? FF 50");
            DALOptions::byte_8709D1 = *pattern.get_first<uint8_t*>(2);

            pattern = hook::pattern("8B 54 24 ? 81 FA ? ? ? ? A1 ? ? ? ? 56");
            DALOptions::shIncScreenRes = safetyhook::create_inline(pattern.get_first(), DALOptions::IncScreenRes);

            pattern = hook::pattern("89 35 ? ? ? ? A1 ? ? ? ? 89 35");
            injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("89 35 ? ? ? ? E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? A1");
            injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("89 35 ? ? ? ? E8 ? ? ? ? 81 3D");
            injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("A1 ? ? ? ? 83 F8 ? 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 8B 44 24");
            shsub_5BF610 = safetyhook::create_inline(pattern.get_first(), GetRacingResolution);

            pattern = hook::pattern("E8 ? ? ? ? 8B 4C 24 ? 83 C4 ? 50 51 E8 ? ? ? ? 8B 47");
            shGetLocalizedString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), GetLocalizedString);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 84 C0 8D 84 24");
            shGetLocalizedWideString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), GetLocalizedWideString);

            pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 83 F8 ? 77 ? FF 24 85 ? ? ? ? 8B 4E");
            DALLangHash::FEngSetLanguageHash = (decltype(DALLangHash::FEngSetLanguageHash))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 5F 81 C4");
            FEString::SetString = (decltype(FEString::SetString))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("56 8B F1 8B 46 ? 68 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 83 F8 ? 77");
            DALLangHash::shGetLocg_RacingResolution = safetyhook::create_inline(pattern.get_first(), DALLangHash::GetLocg_RacingResolution);

            pattern = hook::pattern("6A ? 68 ? ? ? ? 6A ? 6A ? 68 ? ? ? ? 52 FF D6 8B 44 24 ? 6A ? 68 ? ? ? ? 6A");
            static auto RacingResolutionSet = safetyhook::create_mid(pattern.count_hint(8).get(7).get<void*>(7), [](SafetyHookContext& regs)
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

            pattern = hook::pattern("8B 44 24 ? 8D 4C 24 ? 51 68 ? ? ? ? 8D 54 24 ? 52 6A ? 68 ? ? ? ? 50 89 7C 24 ? 89 7C 24 ? FF D6 8B 44 24 ? 8D 4C 24 ? 51 68 ? ? ? ? 8D 54 24 ? 52 6A ? 68 ? ? ? ? 50 89 7C 24 ? 89 7C 24 ? FF D6 8B 4C 24");
            static auto RacingResolutionGet = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
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

            pattern = hook::pattern("E8 ? ? ? ? C6 05 ? ? ? ? ? 39 3D");
            shsub_5D1C40 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), sub_5D1C40);

            pattern = hook::pattern("C7 44 24 ? ? ? ? ? 89 74 24 ? 89 74 24 ? FF D5");
            shWndProc = safetyhook::create_inline(*pattern.get_first<void*>(4), WndProc);

            onResChange() += [](int Width, int Height)
            {
                if (cFEng::pInstance && *cFEng::pInstance)
                    cFEng::MakeLoadedPackagesDirty(*cFEng::pInstance, 0);
            };
        };
    }
} Resolution;