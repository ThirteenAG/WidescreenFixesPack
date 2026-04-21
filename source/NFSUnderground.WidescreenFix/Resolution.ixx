module;

#include <stdafx.h>
#include <d3d9.h>

export module Resolution;

import ComVars;

struct Res
{
    int width;
    int height;
    int refreshRate;
};

std::vector<Res> ResList;
std::vector<std::wstring> ResListText;
std::vector<int> dwSupportedResolutionsArray;
int cachedWidth, cachedHeight;
bool bForceDeviceReset = false;

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

SafetyHookInline shSearchForString = {};
char* __fastcall SearchForString(void* ecx, int hash)
{
    return shSearchForString.unsafe_fastcall<char*>(ecx, hash);
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

std::vector<ProtectedGameRef<int32_t>> ResXRefs;
std::vector<ProtectedGameRef<int32_t>> ResYRefs;
export void SetRes(int width, int height)
{
    for (auto& ref : ResXRefs)
    {
        ref = width;
    }

    for (auto& ref : ResYRefs)
    {
        ref = height;
    }

    cachedWidth = width;
    cachedHeight = height;
    onResChange().executeAll(cachedWidth, cachedHeight);
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

std::string GetResolutionConfigPath()
{
    char basePath[MAX_PATH] = {};

    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, basePath)))
    {
        SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, basePath);
    }

    std::string path = basePath;
    if (!path.empty() && path.back() != '\\' && path.back() != '/')
        path += '\\';

    path += "NFS Underground\\RES.cfg2";

    return path;
}

int GetSavedResolution()
{
    std::string path = GetResolutionConfigPath();

    std::ifstream file(path);
    if (!file.is_open())
        return 0;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.size() >= 4 && line.rfind("RES:", 0) == 0)
        {
            const char* valueStr = line.c_str() + 4;

            while (*valueStr == ' ' || *valueStr == '\t')
                ++valueStr;

            char* endptr = nullptr;
            long value = strtol(valueStr, &endptr, 10);

            if (endptr != valueStr)
            {
                bool valid = true;
                for (char* p = endptr; *p; ++p)
                {
                    if (!isspace(static_cast<unsigned char>(*p)))
                    {
                        valid = false;
                        break;
                    }
                }

                if (valid)
                {
                    return value;
                }
            }
            return 0;
        }
    }

    return 0;
}

void SaveResolution(int packed)
{
    std::string path = GetResolutionConfigPath();

    std::filesystem::path fsPath = path;
    std::error_code ec;
    std::filesystem::create_directories(fsPath.parent_path(), ec);

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open())
        return;

    file << "RES: " << packed << std::endl;
}

void SetResolutionByIndex(int index)
{
    if (index < 0 || index >= (int)ResList.size())
    {
        auto desktopRes = GetDesktopRes();
        SetRes(std::get<0>(desktopRes), std::get<1>(desktopRes));
    }
    else
    {
        SetRes(ResList[index].width, ResList[index].height);
    }
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
                auto label = std::format(L"{}x{}", width, height);
                ResListText.push_back(label);
                dwSupportedResolutionsArray.push_back(1);
            }

            auto pattern = hook::pattern("8B 04 8D ? ? ? ? 85 C0 0F 84");
            auto pSupportedResolutionsArray = *pattern.get_first<void*>(3);

            pattern = hook::pattern("8B 04 8D ? ? ? ? 85 C0 0F 84");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 75 ? 85 C0");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 0C 85 ? ? ? ? 85 C9 74 ? A3 ? ? ? ? 40");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("8B 34 85 ? ? ? ? 33 C9");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("39 0C 85 ? ? ? ? 75");
            injector::AdjustPointer(pattern.get_first(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("39 0C 85 ? ? ? ? 74 ? E8 ? ? ? ? 5E 5B C3");
            injector::AdjustPointer(pattern.count(2).get(0).get<void*>(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);
            injector::AdjustPointer(pattern.count(2).get(1).get<void*>(), dwSupportedResolutionsArray.data(), pSupportedResolutionsArray, pSupportedResolutionsArray);

            pattern = hook::pattern("BA ? ? ? ? 79 ? 89 53");
            injector::WriteMemory(pattern.get_first(1), (int)ResList.size() - 1, true);

            pattern = hook::pattern("BA ? ? ? ? 33 C9 3B C2");
            injector::WriteMemory(pattern.get_first(1), (int)ResList.size() - 1, true);

            pattern = hook::pattern("83 FF ? 0F 8C ? ? ? ? 5F 5E 5D");
            injector::WriteMemory<uint8_t>(pattern.get_first(2), (uint8_t)ResList.size() - 1, true);

            {
                auto setResRef = [&](hook::pattern pat, int offset, std::vector<ProtectedGameRef<int32_t>>& targetVector)
                {
                    pattern.for_each_result([&](const hook::pattern_match& match)
                    {
                        ProtectedGameRef<int32_t> ref;
                        ref.SetAddress(match.get<int32_t>(offset));
                        targetVector.push_back(std::move(ref));
                    });
                };

                // menu
                pattern = hook::pattern("68 ? ? ? ? BE ? ? ? ? C7 05");
                setResRef(pattern, 1, ResXRefs);
                setResRef(pattern, 6, ResYRefs);

                pattern = hook::pattern("68 ? ? ? ? BE ? ? ? ? E8 ? ? ? ? 83 C4 ? 5F");
                setResRef(pattern, 1, ResXRefs);
                setResRef(pattern, 6, ResYRefs);

                pattern = hook::pattern("B8 ? ? ? ? BE ? ? ? ? 77");
                setResRef(pattern, 1, ResXRefs);
                setResRef(pattern, 6, ResYRefs);
                injector::MakeNOP(pattern.get_first(10), 9, true);

                pattern = hook::pattern("B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8");
                setResRef(pattern, 1, ResXRefs);
                setResRef(pattern, 6, ResYRefs);

                pattern = hook::pattern("B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? BE");
                setResRef(pattern, 1, ResXRefs);
                setResRef(pattern, 6, ResYRefs);

                pattern = hook::pattern("B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? BE");
                setResRef(pattern, 1, ResXRefs);
                setResRef(pattern, 6, ResYRefs);

                pattern = hook::pattern("B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? BE");
                setResRef(pattern, 1, ResXRefs);
                setResRef(pattern, 6, ResYRefs);

                pattern = hook::pattern("8B 0D ? ? ? ? 8B 15 ? ? ? ? B8");
                static ProtectedGameRef<int32_t> ScreenWidth;
                ScreenWidth.SetAddress(*pattern.get_first<int32_t*>(2));
                ResXRefs.push_back(ScreenWidth);

                static ProtectedGameRef<int32_t> ScreenHeight;
                ScreenHeight.SetAddress(*pattern.get_first<int32_t*>(8));
                ResYRefs.push_back(ScreenHeight);

                pattern = hook::pattern("89 1D ? ? ? ? 89 1D ? ? ? ? 89 35");
                static ProtectedGameRef<int32_t> ScreenWidth2;
                ScreenWidth2.SetAddress(*pattern.get_first<int32_t*>(2));
                ResXRefs.push_back(ScreenWidth2);

                static ProtectedGameRef<int32_t> ScreenHeight2;
                ScreenHeight2.SetAddress(*pattern.get_first<int32_t*>(14));
                ResYRefs.push_back(ScreenHeight2);
            }

            //pattern = hook::pattern("83 EC ? 55 56 57 33 FF");
            //static auto IncScreenResEvent = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            //{
            //
            //});

            pattern = hook::pattern("D8 25 ? ? ? ? E8 ? ? ? ? 8B 4E");
            static auto IncScreenResEventEnd = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                int index = g_RacingResolution;
                int width = ResList[index].width;
                int height = ResList[index].height;

                if (index >= 0 && index < (int)ResList.size())
                {
                    SetRes(width, height);
                    static int packed = 0;
                    // Pack width and height as shorts into an int, and set sign bit as flag: (width << 16) | height | 0x80000000
                    packed = (width << 16) | (height & 0xFFFF) | 0x80000000;
                    SaveResolution(packed);
                    bForceDeviceReset = true;
                }
            });

            pattern = hook::pattern("89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? E9");
            injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? 75 ? A3 ? ? ? ? 89 0D ? ? ? ? A3");
            injector::MakeNOP(pattern.get_first(), 5, true);

            pattern = hook::pattern("E8 ? ? ? ? 85 C0 74 ? 8B 3D ? ? ? ? 68");
            shSearchForString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SearchForString);

            static auto nCurrentMenuResolutionSelectorIndex = -1;
            pattern = hook::pattern("8B 43 ? 83 C4 ? 83 CE");
            injector::MakeNOP(pattern.get_first(), 3, true);
            static auto Force1280x1024RenderObject = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.eax = 4;
                nCurrentMenuResolutionSelectorIndex = *(int32_t*)(regs.ebx + 0x7C);
            });

            pattern = hook::pattern("8B C3 8D 4C 24 ? ? ? ? ? ? 5F 5E");
            static auto FEngFontRenderStringHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto str = std::wstring_view(*(wchar_t**)(regs.esp + 0x4));
                if (str == L"1280 x 1024")
                {
                    *(wchar_t**)(regs.esp + 0x4) = (wchar_t*)ResListText[nCurrentMenuResolutionSelectorIndex].c_str();
                }
            });

            static int32_t g_RacingResolutionCached = 0;
            pattern = hook::pattern("BB ? ? ? ? 8B 43 ? 8D 54 24");
            static auto RacingResolutionSetStart = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_RacingResolutionCached = g_RacingResolution;
                g_RacingResolution = 0;
            });

            pattern = hook::pattern("55 E8 ? ? ? ? 83 C4 ? 5F 5E 5D 5B 8B E5");
            static auto RacingResolutionSetEnd = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
            {
                int index = g_RacingResolutionCached;
                if (index >= 0 && index < (int)ResList.size())
                {
                    static int packed = 0;
                    int width = ResList[index].width;
                    int height = ResList[index].height;
                    // Pack width and height as shorts into an int, and set sign bit as flag: (width << 16) | height | 0x80000000
                    packed = (width << 16) | (height & 0xFFFF) | 0x80000000;
                    SaveResolution(packed);
                }

                g_RacingResolution = g_RacingResolutionCached;
            });

            static auto unpackRes = [](int packed) -> int
            {
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

                return index;
            };

            pattern = hook::pattern("0F 85 ? ? ? ? 5F 5E 5D 5B 81 C4");
            static auto RacingResolutionGet = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
            {
                g_RacingResolution = unpackRes(GetSavedResolution());
            });

            g_RacingResolution = unpackRes(GetSavedResolution());
            SetResolutionByIndex(g_RacingResolution);

            pattern = hook::pattern("C7 44 24 ? ? ? ? ? 89 5C 24 ? 89 5C 24 ? FF 15");
            shWndProc = safetyhook::create_inline(*pattern.get_first<void*>(4), WndProc);

            static auto loc_40A714 = (uintptr_t)hook::pattern("39 35 ? ? ? ? 74 ? 89 35 ? ? ? ? 33 C0").get_first();
            pattern = hook::pattern("3D ? ? ? ? 75 ? A1 ? ? ? ? ? ? 50 FF 51 ? 3D ? ? ? ? 75 ? E8 ? ? ? ? 39 35");
            injector::MakeNOP(pattern.get_first(), 7, true);
            static auto PresentRetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (bForceDeviceReset)
                    return;

                if (regs.eax != 0x88760868)
                {
                    return_to(loc_40A714);
                }
            });

            pattern = hook::pattern("3D ? ? ? ? 75 ? E8 ? ? ? ? 39 35");
            injector::MakeNOP(pattern.get_first(), 7, true);
            static auto TestCooperativeLevelRetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (bForceDeviceReset)
                {
                    bForceDeviceReset = false;
                    return;
                }

                if (regs.eax != 0x88760869)
                {
                    return_to(loc_40A714);
                }
            });

            onResChange() += [](int Width, int Height)
            {
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

                if (cFEng::pInstance && *cFEng::pInstance)
                    cFEng::MakeLoadedPackagesDirty(*cFEng::pInstance);
            };
        };
    }
} Resolution;