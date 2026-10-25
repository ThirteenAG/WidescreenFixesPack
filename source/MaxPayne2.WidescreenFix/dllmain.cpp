#include "stdafx.h"
#include "FixSettings.hpp"
#include "../Shared/D3D8Compatibility.hpp"
#include "../Shared/CursorConfinement.hpp"
#include "../Shared/MaxPayne/SettingsMenu.hpp"
#include "../Shared/MaxPayne/FrameRate.hpp"

import ComVars;
import e2mfc;
import e2_d3d8_driver_mfc;
import x_basicmodesmfc;
import x_gameobjectsmfc;
import x_helpersmfc;
import x_modesmfc;
import sndmfc;
import x_inputmfc;
import xidi;
import PostFX;
import AntiAliasing;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

// Window mode (MAIN.WindowMode). The game opens its video mode with sub_404B20(windowed), windowed for -window, and the
// window as large as the resolution plus the frame sizes kept at CWnd[27] and CWnd[28], at CWnd[29], CWnd[30]. A window
// larger than the screen, CWnd[23] and CWnd[24], was moved to the corner and shrunk to it, the game rendering at less
// than the resolution. Windowed: the frame of the window's real style, the window centered on its monitor by its
// client area, which a window as large as the monitor covers exactly with the frame outside. Borderless: no frame.
// Full screen with -window keeps the borderless window it had.
namespace WindowMode
{
    enum
    {
        Fullscreen,
        Windowed,
        Borderless,
    };

    // MAIN.WindowMode as it was at startup
    int32_t nWindowMode = Fullscreen;

    // Position and frame size of a window with the client size given, centered on the window's monitor in its DPI
    std::tuple<int32_t, int32_t, int32_t, int32_t> GetCenteredWindowRect(HWND hWnd, int32_t nWidth, int32_t nHeight)
    {
        using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using GetDpi = UINT(WINAPI*)(HWND);
        static auto user32 = GetModuleHandleW(L"user32.dll");
        static auto adjustForDpi = reinterpret_cast<AdjustForDpi>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
        static auto getDpi = reinterpret_cast<GetDpi>(GetProcAddress(user32, "GetDpiForWindow"));

        auto style = (DWORD)GetWindowLongW(hWnd, GWL_STYLE);
        auto exStyle = (DWORD)GetWindowLongW(hWnd, GWL_EXSTYLE);
        RECT rect = { 0, 0, nWidth, nHeight };
        UINT nDpi = getDpi && hWnd ? getDpi(hWnd) : 0;
        if (!(adjustForDpi && nDpi && adjustForDpi(&rect, style, FALSE, exStyle, nDpi)))
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        MONITORINFO info = { sizeof(MONITORINFO) };
        GetMonitorInfoW(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &info);
        auto& monitor = info.rcMonitor;
        int32_t x = monitor.left + (monitor.right - monitor.left - nWidth) / 2 + rect.left;
        int32_t y = monitor.top + (monitor.bottom - monitor.top - nHeight) / 2 + rect.top;
        // a window that fits on the monitor with its frame stays whole on it
        if (rect.right - rect.left <= monitor.right - monitor.left)
            x = std::clamp(x, int32_t(monitor.left), int32_t(monitor.right - (rect.right - rect.left)));
        if (rect.bottom - rect.top <= monitor.bottom - monitor.top)
            y = std::clamp(y, int32_t(monitor.top), int32_t(monitor.bottom - (rect.bottom - rect.top)));
        return { x, y, (rect.right - rect.left) - nWidth, (rect.bottom - rect.top) - nHeight };
    }
}

SafetyHookInline shsub_404B20 = {};
int __fastcall sub_404B20(int* CWnd, void* edx, char a2)
{
    if (WindowMode::nWindowMode != WindowMode::Fullscreen)
        a2 = 1;

    if (a2 && WindowMode::nWindowMode != WindowMode::Windowed)
    {
        // border size to 0
        CWnd[27] = 0;
        CWnd[28] = 0;

        HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX info = { sizeof(MONITORINFOEX) };
        GetMonitorInfo(monitor, &info);
        DEVMODE devmode = {};
        devmode.dmSize = sizeof(DEVMODE);
        EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
        DWORD DesktopX = devmode.dmPelsWidth;
        DWORD DesktopY = devmode.dmPelsHeight;

        // center window position
        CWnd[29] = (int)(((float)DesktopX / 2.0f) - ((float)CWnd[25] / 2.0f));
        CWnd[30] = (int)(((float)DesktopY / 2.0f) - ((float)CWnd[26] / 2.0f));
    }
    else if (a2)
    {
        auto hWnd = *(HWND*)&CWnd[8]; // CWnd::m_hWnd
        auto [x, y, nFrameX, nFrameY] = WindowMode::GetCenteredWindowRect(hWnd, CWnd[25], CWnd[26]);
        CWnd[27] = nFrameX;
        CWnd[28] = nFrameY;
        CWnd[29] = x;
        CWnd[30] = y;
        CWnd[23] = std::max(CWnd[23], CWnd[25] + nFrameX);
        CWnd[24] = std::max(CWnd[24], CWnd[26] + nFrameY);
    }

    return shsub_404B20.unsafe_fastcall<int>(CWnd, edx, a2);
}

// WriteSettingsToFile: the game keeps its settings in the registry through CWinApp's profile
// functions, which keep them in the INI file m_pszProfileName names instead while there's no
// registry key. Instead of setting one, the file next to the savegames gets used, starting out with
// what the registry has the first time.
namespace CWinApp
{
    enum
    {
        APP_NAME = 0x50,     // char* m_pszAppName
        PROFILE_NAME = 0x68, // char* m_pszProfileName
    };

    std::filesystem::path SettingsPath;

    // of the CRT mfc71.dll frees m_pszProfileName with
    char* (__cdecl* crtStrdup)(const char* szString) = nullptr;
    void(__cdecl* crtFree)(void* pBlock) = nullptr;

    // As CWinApp::WriteProfileInt and WriteProfileString would have written the values
    void CopyRegistrySettings(const std::string& szKey)
    {
        HKEY hApp = nullptr;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, szKey.c_str(), 0, KEY_READ, &hApp) != ERROR_SUCCESS)
            return;

        auto szFile = SettingsPath.string();
        char szSection[256];
        for (DWORD i = 0; ; ++i)
        {
            DWORD nSectionLength = sizeof(szSection);
            if (RegEnumKeyExA(hApp, i, szSection, &nSectionLength, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                break;

            HKEY hSection = nullptr;
            if (RegOpenKeyExA(hApp, szSection, 0, KEY_READ, &hSection) != ERROR_SUCCESS)
                continue;

            char szName[256];
            char data[4096];
            for (DWORD j = 0; ; ++j)
            {
                DWORD nNameLength = sizeof(szName);
                DWORD nDataSize = sizeof(data) - 1;
                DWORD nType = REG_NONE;
                auto status = RegEnumValueA(hSection, j, szName, &nNameLength, nullptr, &nType, (BYTE*)data, &nDataSize);
                if (status == ERROR_NO_MORE_ITEMS)
                    break;
                if (status != ERROR_SUCCESS)
                    continue;

                if (nType == REG_DWORD && nDataSize == sizeof(DWORD))
                    WritePrivateProfileStringA(szSection, szName, std::to_string(*(int32_t*)data).c_str(), szFile.c_str());
                else if (nType == REG_SZ)
                {
                    data[nDataSize] = '\0';
                    WritePrivateProfileStringA(szSection, szName, data, szFile.c_str());
                }
            }
            RegCloseKey(hSection);
        }
        RegCloseKey(hApp);
    }

    SafetyHookInline shSetRegistryKey = {};
    void __fastcall SetRegistryKey(uint8_t* _this, void* edx, const char* szRegistryKey)
    {
        std::error_code ec;
        if (!std::filesystem::exists(SettingsPath, ec))
            CopyRegistrySettings(std::string("Software\\") + szRegistryKey + "\\" + *(const char**)(_this + APP_NAME));

        auto& szProfileName = *(char**)(_this + PROFILE_NAME);
        crtFree(szProfileName);
        szProfileName = crtStrdup(SettingsPath.string().c_str());
    }
}

void InitSettingsFile()
{
    auto& settings = FixSettings();
    if (!settings.Bool("MAIN.PortableSettings"))
        return;

    char szDocuments[MAX_PATH];
    if (settings.Bool("MISC.UseGameFolderForSavegames"))
        CWinApp::SettingsPath = GetExeModulePath() / "savegames";
    else if (SHGetSpecialFolderPathA(nullptr, szDocuments, CSIDL_PERSONAL, FALSE))
        CWinApp::SettingsPath = std::filesystem::path(szDocuments) / "Max Payne 2 Savegames";
    else
        return;

    std::error_code ec;
    std::filesystem::create_directories(CWinApp::SettingsPath, ec);
    CWinApp::SettingsPath /= "settings.ini";

    auto crt = GetModuleHandleA("msvcr71.dll");
    CWinApp::crtStrdup = (decltype(CWinApp::crtStrdup))GetProcAddress(crt, "_strdup");
    CWinApp::crtFree = (decltype(CWinApp::crtFree))GetProcAddress(crt, "free");
    if (!CWinApp::crtStrdup || !CWinApp::crtFree)
        return;

    CWinApp::shSetRegistryKey = safetyhook::create_inline(GetProcAddress(GetModuleHandleA("mfc71.dll"), MAKEINTRESOURCEA(5975)), CWinApp::SetRegistryKey); // CWinApp::SetRegistryKey(LPCTSTR)
}

// The display modes the startup dialog's options list, in the order the adapter enumerates them:
// smallest first. They're listed largest first instead.
namespace DisplayModeList
{
    enum
    {
        IDC_DISPLAY_MODE = 1002,
        CWND_HWND = 0x20, // CWnd::m_hWnd
    };

    // What the items point to, the game reads the choice back from it
    struct Mode
    {
        uint32_t nWidth;
        uint32_t nHeight;
        uint32_t nBitsPerPixel;
    };

    void Sort(HWND hComboBox)
    {
        struct Item
        {
            std::string text;
            const Mode* pMode;
        };

        std::vector<Item> Items;
        const Mode* pSelected = nullptr;
        auto nCount = (int)SendMessageA(hComboBox, CB_GETCOUNT, 0, 0);
        auto nSelected = (int)SendMessageA(hComboBox, CB_GETCURSEL, 0, 0);
        for (int i = 0; i < nCount; ++i)
        {
            auto pMode = (const Mode*)SendMessageA(hComboBox, CB_GETITEMDATA, i, 0);
            auto nLength = (int)SendMessageA(hComboBox, CB_GETLBTEXTLEN, i, 0);
            if (!pMode || pMode == (const Mode*)CB_ERR || nLength == CB_ERR)
                return;

            std::string text(nLength, '\0');
            SendMessageA(hComboBox, CB_GETLBTEXT, i, (LPARAM)text.data());
            if (i == nSelected)
                pSelected = pMode;
            Items.push_back({ std::move(text), pMode });
        }

        std::stable_sort(Items.begin(), Items.end(), [](const Item& a, const Item& b)
        {
            return std::tie(a.pMode->nWidth, a.pMode->nHeight, a.pMode->nBitsPerPixel) > std::tie(b.pMode->nWidth, b.pMode->nHeight, b.pMode->nBitsPerPixel);
        });

        SendMessageA(hComboBox, CB_RESETCONTENT, 0, 0);
        auto nNewSelected = CB_ERR;
        for (auto& item : Items)
        {
            // unlike CB_ADDSTRING, keeps the order even if the list sorts itself
            auto nIndex = (int)SendMessageA(hComboBox, CB_INSERTSTRING, -1, (LPARAM)item.text.c_str());
            SendMessageA(hComboBox, CB_SETITEMDATA, nIndex, (LPARAM)item.pMode);
            if (item.pMode == pSelected)
                nNewSelected = nIndex;
        }

        SendMessageA(hComboBox, CB_SETCURSEL, nNewSelected == CB_ERR ? 0 : nNewSelected, 0);
    }

    // Fills the dialog's lists for the adapter picked
    SafetyHookInline shFill = {};
    int __fastcall Fill(uint8_t* _this, void* edx)
    {
        auto result = shFill.unsafe_fastcall<int>(_this, edx);
        if (auto hComboBox = GetDlgItem(*(HWND*)(_this + CWND_HWND), IDC_DISPLAY_MODE))
            Sort(hComboBox);
        return result;
    }
}

// The settings that apply at once, read again whenever one of them changes (in the menu, in an editor through the
// live link, or the graphic novel key)
void ApplySettings()
{
    auto& settings = FixSettings();
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(settings.String("MAIN.HudAspectRatioConstraint"));
    Screen.fFOVFactor = settings.Float("MAIN.FOVFactor");
    if (Screen.fFOVFactor <= 0.0f) { Screen.fFOVFactor = 1.0f; }
    Screen.bGraphicNovelMode = settings.Int("MAIN.GraphicNovelMode") != 0;
    Cinematic::nCutsceneBorders = std::clamp(settings.Int("MAIN.CutsceneBorders"), (int32_t)Cinematic::Off, (int32_t)Cinematic::Both);
    Cinematic::bNoBorderAnimation = !settings.Bool("MAIN.AnimatedCutsceneBorders");
    nGraphicNovelModeKey = MaxPayneMenu::KeyCode(settings.String("MAIN.GraphicNovelModeKey"));

    // HUD offsets, cameras and the graphic novel page follow the new values
    RecalculateScreen();
}

void ReadSettings()
{
    ApplySettings();
    for (auto szId : { "MAIN.HudAspectRatioConstraint", "MAIN.FOVFactor", "MAIN.GraphicNovelMode", "MAIN.CutsceneBorders", "MAIN.AnimatedCutsceneBorders", "MAIN.GraphicNovelModeKey" })
        FixSettings().OnChange(szId, ApplySettings);
}

// The fix's settings next to the game's options; the rest is on its page in Plugins
std::vector<MaxPayneMenu::Placement> MenuPlacements()
{
    using MaxPayneMenu::Spot;
    return
    {
        { "GCID_VIDEOU", Spot::BeforeLastMenu, 2, { "MAIN.WindowMode", "MAIN.FOVFactor", "MAIN.HudAspectRatioConstraint", "GRAPHICS.AntiAliasing", "GRAPHICS.ConsoleGamma", "GRAPHICS.VSync", "MAIN.FPSLimit" } },
        { "GCID_GENERICU", Spot::BeforeFirstItem, 0, { "MAIN.CutsceneBorders", "MAIN.AnimatedCutsceneBorders", "MAIN.GraphicNovelMode", "MAIN.FMVWidescreenMode" } },
        { "GCID_CONTROLSU", Spot::BeforeLastMenu, 3, { "MAIN.GamepadIcons" } },
        { "GCID_MOUSESETTINGSU", Spot::BeforeBack, 3, { "MAIN.MouseSmoothing" } },
    };
}

// The fix's rows in the game's Options menu, see source/Shared/MaxPayne/SettingsMenu.hpp. MP_MenuMode
// answers the rows through its virtual functions setItem, isItemEnabled, setSlider and getSlider; the item and slider
// lines (X_BasicModesMFC) keep their name (P_Text) at +4 and their ID at +12, and update in slot 3.
void InitMenu()
{
    HMODULE hModule = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&InitMenu, &hModule);
    auto basicModes = GetModuleHandleW(L"X_BasicModesMFC");
    if (!basicModes || !MaxPayneMenu::Init(hModule, GetModuleHandleW(L"rlmfc"), GetModuleHandleW(L"e2mfc"), MenuPlacements()))
        return;

    MaxPayneMenu::HookMenu(MaxPayneMenu::FindVTable(GetModuleHandleW(nullptr), ".?AVMP_MenuMode@@"), { 21, 22, 29, 30 },
        MaxPayneMenu::FindVTable(basicModes, ".?AVX_MenuItem@@"), MaxPayneMenu::FindVTable(basicModes, ".?AVX_MenuSlider@@"), { 4, 12, 3 });
}

void Init()
{
    auto& settings = FixSettings();
    static bool bUseGameFolderForSavegames = settings.Bool("MISC.UseGameFolderForSavegames");
    if (bUseGameFolderForSavegames)
    {
        auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 40 04 68");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x41D14C
    }

    // Losing the focus pauses the game, unless it keeps running in the background
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B CF C6 87 82 00 00 00 00");
        static auto pKillFocusCall = (uintptr_t)pattern.get_first(0); //0x404935
        static auto KillProgramFocusHook = safetyhook::create_mid(pKillFocusCall, [](SafetyHookContext& regs)
        {
            static auto& nPauseOnFocusLoss = FixSettings().Int("MAIN.PauseOnFocusLoss");
            if (!nPauseOnFocusLoss)
                return_to(pKillFocusCall + 5);
        });
    }

    static int32_t nLoadSaveSlot = settings.Int("MISC.LoadSaveSlot");
    if (nLoadSaveSlot == -2 || nLoadSaveSlot == -3 || (nLoadSaveSlot >= 0 && nLoadSaveSlot <= 999))
    {
        static auto unk_556860 = *hook::get_pattern<void*>("BA ? ? ? ? 2B D1 8A 01 88 04 0A", 1);
        if (*(uint8_t*)unk_556860 == 0)
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8D 45 18 50");
            static auto AfxGetModuleState = injector::GetBranchDestination(pattern.get_first(0), true); //0x4D2DB2
            static auto GetProfileStringA = injector::GetBranchDestination(pattern.get_first(27), true); //0x4D2ECC
            static auto aLastSavedGameF = *pattern.get_first<char*>(14); // 4E8F44
            static auto aSaveGame = *pattern.get_first<char*>(19); // 4E62D4
            static auto aSavegames = *hook::get_pattern<char*>("8B 15 ? ? ? ? 89 17 A1 ? ? ? ? 8B 55 10 89 47 04", 2);
            static auto MaxPayne2Saveg = *hook::get_pattern<char*>("8B 15 ? ? ? ? 89 17 A1 ? ? ? ? 89 47 04", 2);
            pattern = hook::pattern("89 97 ? ? ? ? FF 15 ? ? ? ? FF 15 ? ? ? ? E8 ? ? ? ? 8D 50 01");
            struct SaveGameHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.edi + 0xE8) = regs.edx;

                    if (nLoadSaveSlot == -2)
                    {
                        void* pStr = nullptr;
                        auto _this = injector::stdcall<void* ()>::call(AfxGetModuleState);
                        injector::thiscall<void(void* _this, void* out, char const* a1, char const* a2, char const* a3)>::call(GetProfileStringA, *(void**)((uint32_t)_this + 4), &pStr, (char const*)aSaveGame, (char const*)aLastSavedGameF, "");
                        std::string_view LastSavedGameFilename{ (char*)pStr };
                        if (!LastSavedGameFilename.empty())
                            injector::WriteMemoryRaw(unk_556860, (void*)LastSavedGameFilename.data(), LastSavedGameFilename.size(), true);
                    }
                    else
                    {
                        char buffer[MAX_PATH];
                        if (bUseGameFolderForSavegames)
                        {
                            GetModuleFileNameA(NULL, buffer, MAX_PATH);
                            *strrchr(buffer, '\\') = '\0';
                            strcat_s(buffer, "\\");
                            strcat_s(buffer, aSavegames);
                            strcat_s(buffer, "\\");
                        }
                        else
                        {
                            SHGetSpecialFolderPathA(0, buffer, 5, false);
                            strcat_s(buffer, "\\");
                            strcat_s(buffer, MaxPayne2Saveg);
                            strcat_s(buffer, "\\");
                        }

                        auto nSaveNum = -1;
                        std::string SFPath(buffer);

                        WIN32_FIND_DATAA fd;
                        HANDLE File = FindFirstFileA(std::string(SFPath + "*.mp2s").c_str(), &fd);
                        FILETIME LastWriteTime = fd.ftLastWriteTime;

                        if (File != INVALID_HANDLE_VALUE)
                        {
                            do
                            {
                                std::string str(fd.cFileName);
                                auto n = str.find_first_of("0123456789");

                                if (nLoadSaveSlot >= 0)
                                {
                                    if (n != std::string::npos)
                                    {
                                        nSaveNum = std::atoi(&str[n]);
                                        if (nSaveNum == nLoadSaveSlot)
                                        {
                                            SFPath += str;
                                            injector::WriteMemoryRaw(unk_556860, SFPath.data(), SFPath.size(), true);
                                            return;
                                        }
                                    }
                                }
                                else
                                {
                                    if (CompareFileTime(&fd.ftLastWriteTime, &LastWriteTime) >= 0)
                                    {
                                        LastWriteTime = fd.ftLastWriteTime;
                                        std::string str(fd.cFileName);
                                        if (n != std::string::npos)
                                        {
                                            nSaveNum = std::atoi(&str[n]);
                                        }
                                    }
                                }
                            } while (FindNextFileA(File, &fd));
                            FindClose(File);
                        }

                        if (nSaveNum >= 0 && nLoadSaveSlot < 0)
                        {
                            char buffer[5]; sprintf(buffer, "%03d", nSaveNum);
                            SFPath += fd.cFileName;
                            auto offset = SFPath.end() - std::strlen("000.mp2s");
                            SFPath.replace(offset, offset + 3, buffer);
                            injector::WriteMemoryRaw(unk_556860, SFPath.data(), SFPath.size(), true);
                        }
                    }
                }
            }; injector::MakeInline<SaveGameHook>(pattern.get_first(0), pattern.get_first(6)); //0x4187C3
        }
    }

    // MP_GameMode render function, where the game view gets rendered
    auto pattern = hook::pattern("C6 86 48 01 00 00 01 FF 15 ? ? ? ? 8B 8E A0 10 00 00 FF 15");
    static auto MP_GameModeRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) //0x451B93
    {
        MP_GameMode::pInstance = (uint8_t*)regs.esi;
        bGameViewRendered = true;
        AntiAliasing::BeginGameView();
        Cinematic::UpdateBorders(MP_GameMode::pInstance);
    });

    // MP_GameMode destructor. The game mode is deleted on quit, while the progress bar still draws.
    pattern = hook::pattern("C7 06 ? ? ? ? C7 46 19 ? ? ? ? C7 86 BE 00 00 00 ? ? ? ? C7 44 24 24 19 00 00 00");
    static auto MP_GameModeDestructorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) //0x46A412
    {
        if ((uint8_t*)regs.esi == MP_GameMode::pInstance)
            MP_GameMode::pInstance = nullptr;
    });

    // Cutscene borders: every height multiplier read in MP_HUDMode's render function, which moves
    // subtitles clear of the letterbox bars and then draws them
    pattern = hook::pattern("8B 8E AE 00 00 00 33 DB 89 5C 24 ? E8"); // subtitle offset check
    injector::MakeCALL(pattern.get_first(12), Cinematic::GetBordersHeightMultiplier, true); //0x488A01
    pattern = hook::pattern("8B 8E AE 00 00 00 E8 ? ? ? ? D8 2D ? ? ? ? 8B 8E 07 02 00 00"); // subtitle offset
    injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x488A1D
    pattern = hook::pattern("FF 52 0C 8B 8E AE 00 00 00 E8"); // subtitle offset restore check
    injector::MakeCALL(pattern.get_first(9), Cinematic::GetBordersHeightMultiplier, true); //0x488B23
    pattern = hook::pattern("8B 8E AE 00 00 00 E8 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 05 0F 8A ? ? ? ? 8B 15"); // bars check
    injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x488C05
    pattern = hook::pattern("8B 8E AE 00 00 00 D9 5C 24 10 E8"); // bar height
    injector::MakeCALL(pattern.get_first(10), Cinematic::GetBordersHeightMultiplier, true); //0x488C68

    // MP_HUDMode's render function updates the fade layer right before drawing it
    pattern = hook::pattern("8B 8E F0 02 00 00 D9 1C 24 E8");
    static auto MaxPayne_HUDFadeLayerHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs) //0x4889F0
    {
        if (regs.ecx)
            MaxPayne_HUDFadeLayer::pSprite = *(uint8_t**)(regs.ecx + 0xAD);
    });

    // Graphic novels: key toggles between the original framing and the whole page as large as the screen allows
    pattern = hook::pattern("56 57 8B F1 E8 ? ? ? ? 8B 7C 24 0C 8D 46 37 50 8D 4F 38 E8"); // MaxPayne_GraphicNovelPage::show
    MaxPayne_GraphicNovelPage::shShow = safetyhook::create_inline(pattern.get_first(), MaxPayne_GraphicNovelPage::show); //0x4862D0

    // Graphic novel cursor bounds, see UpdateCursorBounds
    pattern = hook::pattern("52 68 00 00 20 44 E8 ? ? ? ? 51 D9 1C 24 6A 00 E8 ? ? ? ? D9 5C 24 ? 8B 44 24 ? 50 68 00 00 F0 43 E8 ? ? ? ? 51 D9 1C 24 6A 00 E8"); // MP_GraphicNovelMode::update
    injector::MakeCALL(pattern.get_first(6), ClampCursorRight, true); //0x484FB5
    injector::MakeCALL(pattern.get_first(17), ClampCursorLeft, true);
    injector::MakeCALL(pattern.get_first(36), ClampCursorBottom, true);
    injector::MakeCALL(pattern.get_first(47), ClampCursorTop, true);

    InitPostFX();

    // Startup dialog, see DisplayModeList
    pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 50 53 55 56 57 8B F1 68 E8 03 00 00 89 74 24 1C E8");
    DisplayModeList::shFill = safetyhook::create_inline(pattern.get_first(), DisplayModeList::Fill); //0x40C420

    // Post-processing (pain and bullet time): the scene is warped with a grid covering the render target
    pattern = hook::pattern("8B 8E 9C 00 00 00 52 68 ? ? ? ? E8");
    static auto PostProcessWarpHook = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs) //0x47AB87
    {
        KeepOriginalProjection(*(void**)(regs.ecx + 0x10));
        AntiAliasing::OnPostProcessing();
    });

    //savegame date format
    static auto fmt = settings.String("MISC.SaveStringFormat");
    pattern = hook::pattern("68 ? ? ? ? 8D 54 24 18 68 ? ? ? ? 52"); //41CA8D
    injector::WriteMemory(pattern.get_first(1), fmt.data(), true);

    // Window mode, see sub_404B20
    pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 14 53 55");
    shsub_404B20 = safetyhook::create_inline(pattern.get_first(), sub_404B20);

    // Skip intro: the logo movie (startup.bik) at startup is skipped the way the -skipstartup switch does it
    if (settings.Bool("MAIN.SkipIntro"))
    {
        pattern = hook::pattern("E8 ? ? ? ? 84 C0 75 3E 8B 8F A8 00 00 00"); // MaxPayneCommandLine::skipStartup()
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(7), 0xEB, true); // jnz -> jmp //0x4188EA
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        D3D8Compatibility::Initialize(true);
        // Per monitor DPI awareness, so the game gets real pixels on scaled desktops. The process' awareness can be set
        // only once: when it's set already (the exe's compatibility settings, "Override high DPI scaling", make it per
        // monitor v1 or system aware before any plugin loads), the windows this thread creates (the game's) get it
        using SetContext = BOOL(WINAPI*)(HANDLE);
        using SetThreadContext = HANDLE(WINAPI*)(HANDLE);
        auto user32 = GetModuleHandleW(L"user32.dll");
        if (auto set = reinterpret_cast<SetContext>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")))
        {
            if (!set(reinterpret_cast<HANDLE>(-4)))
            {
                if (auto setThread = reinterpret_cast<SetThreadContext>(GetProcAddress(user32, "SetThreadDpiAwarenessContext")))
                    setThread(reinterpret_cast<HANDLE>(-4));
            }
        }
        else
            SetProcessDPIAware();

        // Settings described in settings.json and stored in the plugin's .cfg; editors change them through the live link
        FixSettings();
        WindowMode::nWindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), (int32_t)WindowMode::Fullscreen, (int32_t)WindowMode::Borderless);
        asi_settings::LiveLink::Start();
        AntiAliasing::Init();
        ReadSettings();
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []() { CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor")); });
        InitSettingsFile();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 40 04 68"));
        CallbackHandler::RegisterCallback(L"E2MFC.dll", InitE2MFC);
        CallbackHandler::RegisterCallback(L"X_GameObjectsMFC.dll", InitX_GameObjectsMFC);
        CallbackHandler::RegisterCallback(L"X_ModesMFC.dll", InitX_ModesMFC);
        CallbackHandler::RegisterCallback(L"X_HelpersMFC.dll", InitX_HelpersMFC);
        CallbackHandler::RegisterCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { InitE2_D3D8_DRIVER_MFC(); InitPostFXDriver(); });
        CallbackHandler::RegisterModuleUnloadCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { BorderlessWindowedHook.reset(); shDllMainHook.reset(); ShutdownE2_D3D8_DRIVER_MFC(); ShutdownPostFXDriver(); });
        CallbackHandler::RegisterCallback(L"X_BasicModesMFC.dll", []() { InitX_BasicModesMFC(); InitMenu(); });
        CallbackHandler::RegisterCallback(L"sndmfc.dll", InitSNDMFC);
        CallbackHandler::RegisterCallback(L"X_Inputmfc.dll", InitInput);
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        CursorConfinement::Shutdown();
    }
    return TRUE;
}
