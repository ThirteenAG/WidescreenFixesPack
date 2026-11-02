module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module Misc;

import ComVars;
import Frontend;
import Mouse;
import Movies;
import Runtime;

// The screen's numbers for the HUD and the camera, from the resolution and the settings that apply at once
void UpdateScreen()
{
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudScaleX = (480.0f * Screen.fAspectRatio) / 640.0f;
    Screen.fHudOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) / (Screen.fWidth / 640.0f);
    Screen.fDynamicScreenFieldOfViewScale = 0.75f * (4.0f / 3.0f) / Screen.fAspectRatio;
    fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f);
    if (fHudAspectRatioConstraint.has_value())
    {
        float value = fHudAspectRatioConstraint.value();
        if (value < 0.0f || value > (32.0f / 9.0f))
            fWidescreenHudOffset = value;
        else
        {
            value = ClampHudAspectRatio(value, Screen.fAspectRatio);
            fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f);
        }
    }
    fWidescreenHudOffset /= Screen.fAspectRatio / (4.0f / 3.0f);
}

// The settings that apply at once, read again whenever one of them changes (in the menu, or in an editor through the
// live link)
void ApplySettings()
{
    auto& settings = FixSettings();
    CursorConfinement::SetEnabled(settings.Bool("MAIN.ConfineCursor"));
    nCutsceneBorders = std::clamp(settings.Int("MAIN.CutsceneBorders"), 0, 3);
    AnimatedCutsceneBorders = settings.Bool("MAIN.AnimatedCutsceneBorders");
    nFMVWidescreenMode = std::clamp(settings.Int("MAIN.FMVWidescreenMode"), 0, 1);
    fHudAspectRatioConstraint = ParseWidescreenHudOffset(settings.String("MAIN.HudAspectRatioConstraint"));
    fFOVFactor = std::clamp(settings.Float("MAIN.FOVFactor"), 0.5f, 2.0f);
    if (Screen.Width && Screen.Height)
        UpdateScreen();
}

export void ReadSettings()
{
    WindowedModeWrapper::EnableDpiAwareness();
    nWindowedMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    ApplySettings();
    for (auto szId : { "MAIN.CutsceneBorders", "MAIN.AnimatedCutsceneBorders", "MAIN.FMVWidescreenMode", "MAIN.HudAspectRatioConstraint", "MAIN.FOVFactor", "MAIN.ConfineCursor" })
        FixSettings().OnChange(szId, ApplySettings);
}

// The resolution, kept in the game's own res.bin. The exe stores an index into its list of five there (retail 640x480
// to 1600x1200, GOG 1280x720 to 3840x2160) and reads it back at start. The fix stores the size itself instead,
// 0x80000000 | (width << 16) | height, which the exe without the fix doesn't know and replaces with its 1024x768
// default; the same value is what the game keeps as the current width (dword_761208) and in the Screen Configuration
// page, which steps through every mode of the display. Changes apply after a restart, as in the game.
namespace Resolution
{
    struct Mode
    {
        int32_t nWidth;
        int32_t nHeight;
        bool operator==(const Mode&) const = default;
    };

    constexpr uint32_t Packed = 0x80000000;
    std::vector<Mode> Modes;            // of the display, smallest first
    std::array<Mode, 5> LegacyModes;    // the exe's own list
    bool bResolved = false;

    bool IsPacked(uint32_t value)
    {
        return (value & Packed) != 0;
    }

    uint32_t Pack(const Mode& mode)
    {
        return Packed | (uint32_t(mode.nWidth & 0x7FFF) << 16) | uint32_t(mode.nHeight & 0xFFFF);
    }

    Mode Unpack(uint32_t value)
    {
        return { int32_t((value >> 16) & 0x7FFF), int32_t(value & 0xFFFF) };
    }

    Mode Desktop()
    {
        auto [nWidth, nHeight] = GetDesktopRes();
        return { nWidth, nHeight };
    }

    // Every size the display offers from 640x480 up, and the desktop's
    void FindModes()
    {
        Modes.clear();
        DEVMODEW devmode = { .dmSize = sizeof(DEVMODEW) };
        for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &devmode); ++i)
        {
            Mode mode{ int32_t(devmode.dmPelsWidth), int32_t(devmode.dmPelsHeight) };
            if (devmode.dmBitsPerPel >= 16 && mode.nWidth >= 640 && mode.nHeight >= 480 && std::ranges::find(Modes, mode) == Modes.end())
                Modes.push_back(mode);
        }
        if (auto desktop = Desktop(); std::ranges::find(Modes, desktop) == Modes.end())
            Modes.push_back(desktop);
        std::ranges::sort(Modes, [](const Mode& a, const Mode& b) { return std::tie(a.nWidth, a.nHeight) < std::tie(b.nWidth, b.nHeight); });
    }

    bool Usable(const Mode& mode)
    {
        if (mode.nWidth < 640 || mode.nHeight < 480)
            return false;
        // A window can have any size that fits the desktop; full screen needs a mode of the display
        if (nWindowedMode)
        {
            auto desktop = Desktop();
            return mode.nWidth <= desktop.nWidth && mode.nHeight <= desktop.nHeight;
        }
        return std::ranges::find(Modes, mode) != Modes.end();
    }

    // The resolution of this run: ResX/ResY of the settings file, or res.bin's (saved: nullopt without res.bin), or the
    // desktop's. An index of the exe's list is the GOG version's own choice; on the retail exe it's the one the old fix
    // ignored (it always used the desktop's), so the desktop's again.
    void Resolve(std::optional<uint32_t> saved)
    {
        auto& settings = FixSettings();
        Mode mode = Desktop();
        Mode forced{ settings.Int("MAIN.ResX"), settings.Int("MAIN.ResY") };
        if (forced.nWidth > 0 && forced.nHeight > 0)
            mode = forced;
        else if (saved)
        {
            Mode chosen{};
            if (IsPacked(*saved))
                chosen = Unpack(*saved);
            else if (bGOG && *saved < LegacyModes.size())
                chosen = LegacyModes[*saved];
            if (Usable(chosen))
                mode = chosen;
        }
        if (!Usable(mode) || nWindowedMode == 2)
            mode = Desktop();
        if (mode.nWidth <= 0 || mode.nHeight <= 0)
            mode = { 640, 480 };
        Screen.Width = mode.nWidth;
        Screen.Height = mode.nHeight;
        UpdateScreen();
        bResolved = true;
    }

    // sub_4092B0: writes res.bin, from the startup (no res.bin yet: the default width) and the Screen Configuration's Accept
    void __cdecl WriteResBin(uint32_t value)
    {
        if (!IsPacked(value))
        {
            if (!bResolved)
                Resolve(std::nullopt);
            value = Pack({ Screen.Width, Screen.Height });
        }
        FILE* file = nullptr;
        if (fopen_s(&file, "res.bin", "wb") == 0 && file)
        {
            std::fwrite(&value, sizeof(value), 1, file);
            std::fclose(file);
        }
    }

    // The Screen Configuration's next (230) and previous (210) resolution; 220 only shows it
    void Step(uint32_t& current, int32_t nCode)
    {
        if (!IsPacked(current))
            current = Pack({ Screen.Width, Screen.Height });
        if ((nCode != 230 && nCode != 210) || Modes.empty())
            return;
        auto mode = Unpack(current);
        auto it = std::ranges::lower_bound(Modes, mode, [](const Mode& a, const Mode& b) { return std::tie(a.nWidth, a.nHeight) < std::tie(b.nWidth, b.nHeight); });
        int nCount = int(Modes.size());
        int nIndex = int(it - Modes.begin());
        bool bListed = it != Modes.end() && *it == mode;
        if (nCode == 230)
            nIndex = bListed ? nIndex + 1 : nIndex;
        else
            nIndex = nIndex - 1;
        current = Pack(Modes[((nIndex % nCount) + nCount) % nCount]);
    }

    void Init()
    {
        FindModes();

        auto switchPattern = hook::pattern("8B 45 F0 83 C4 14 83 F8 04 77 ? FF 24 85");
        auto afterPattern = hook::pattern("84 DB 89 35 ? ? ? ? 75");
        auto writePattern = hook::pattern("55 8B EC 8B 45 08 3D ? ? ? ? 7F 2B 74 20 3D");
        auto stepPattern = hook::pattern("81 FF E6 00 00 00 B9 ? ? ? ? BA ? ? ? ? BE");
        auto labelPattern = hook::pattern("8B 8B B8 D3 00 00 3B CE 0F 8F");

        // InitInstance: res.bin read, the switch over its index (its sizes tell the GOG exe from the retail one)
        auto pSwitch = switchPattern.get_first<uint8_t>(6); //0x4021D9
        auto pCases = switchPattern.get_first<uint8_t>(18); //0x4021E5, mov esi, w / mov edi, h / jmp, 12 bytes each
        for (size_t i = 0; i < LegacyModes.size(); ++i)
            LegacyModes[i] = { *(int32_t*)(pCases + i * 12 + 1), *(int32_t*)(pCases + i * 12 + 6) };
        // GOG's list starts at 1280x720
        if (!hook::pattern("8B 45 F0 83 C4 14 83 F8 04 77 ? FF 24 85 ? ? ? ? BE 00 05 00 00").empty())
            bGOG = true;

        auto pAfterSwitch = afterPattern.get_first<uint8_t>(0); //0x40222A
        dword_761208.SetAddress(*afterPattern.get_first<uint32_t*>(4)); //dword_761208

        struct ReadResBinHook
        {
            void operator()(injector::reg_pack& regs)
            {
                Resolve(regs.eax);
                regs.esi = Screen.Width;
                regs.edi = Screen.Height;
            }
        }; injector::MakeInline<ReadResBinHook>(pSwitch, pCases); //0x4021D9, 0x4021E5
        injector::MakeJMP(pCases, pAfterSwitch, true);

        // Both ways (with and without res.bin) set the current width here
        struct CurrentResolutionHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (!bResolved)
                    Resolve(std::nullopt);
                regs.esi = Screen.Width;
                regs.edi = Screen.Height;
                dword_761208 = Pack({ Screen.Width, Screen.Height });
            }
        }; injector::MakeInline<CurrentResolutionHook>(pAfterSwitch + 2, pAfterSwitch + 8); //0x40222C, 0x402232

        injector::MakeJMP(writePattern.get_first(0), WriteResBin, true); //0x4092B0

        // Screen Configuration: [menu+0xD3B8] keeps the resolution picked, the packed value
        auto pStep = stepPattern.get_first<uint8_t>(0); //0x40C750
        struct StepResolutionHook
        {
            void operator()(injector::reg_pack& regs)
            {
                Step(*(uint32_t*)(regs.ebx + 0xD3B8), int32_t(regs.edi));
            }
        }; injector::MakeInline<StepResolutionHook>(pStep, pStep + 21); //0x40C750, 0x40C765
        injector::MakeJMP(pStep + 21, pStep + 0x43, true); //0x40C765 -> 0x40C793, the label

        // The label's "%dx%d" (eax: the format)
        auto pLabel = labelPattern.get_first<uint8_t>(0); //0x40C7C3
        struct LabelHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto mode = Unpack(*(uint32_t*)(regs.ebx + 0xD3B8));
                regs.ecx = mode.nWidth;
                regs.edx = mode.nHeight;
            }
        }; injector::MakeInline<LabelHook>(pLabel, pLabel + 14); //0x40C7C3, 0x40C7D1
        injector::MakeJMP(pLabel + 14, pLabel + 0xD1, true); //0x40C7D1 -> 0x40C894, push edx; push ecx; sprintf
    }
}

// Windowed and borderless: the game opens a popup window of the resolution's size and RenderWare's full screen video
// mode (its -Windowed switch takes another, MFC way that doesn't run the game). The fix centers the window (with a
// frame in windowed mode) and has RenderWare start its windowed video mode instead, which sizes Direct3D's back buffer
// after the window. The cursor is kept in the window while the game is active, as GOG's version does for full screen.
namespace Windowed
{
    HWND hGameWindow = nullptr;

    // Direct3D's windowed present parameters (sub_6354B0 without its full screen flag): the back buffer takes the
    // window's size (the exe compared the window's left with its right instead, GOG's version fixed it too)
    SafetyHookInline shFillPresentParameters = {};
    void __cdecl FillPresentParameters(int* pMode, int nFlags, int nFormat)
    {
        shFillPresentParameters.unsafe_ccall(pMode, nFlags, nFormat);
        if ((nFlags & 1) || !dword_8A2620.is_initialized())
            return;
        dword_8A2620->BackBufferWidth = 0;
        dword_8A2620->BackBufferHeight = 0;
        dword_8A2620->BackBufferFormat = D3DFMT_UNKNOWN;
    }

    // The game's window (class MWS, a popup of the resolution's size at 0, 0): centered on the desktop, with a frame
    // in windowed mode. MFC creates it through the exe's import.
    HWND WINAPI CreateWindowExAHook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        bool gameWindow = !hWndParent && lpClassName && HIWORD(lpClassName) && !std::strcmp(lpClassName, "MWS");
        if (!gameWindow)
            return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        WindowedModeWrapper::bBorderlessWindowed = nWindowedMode == 2;
        WindowedModeWrapper::bStretchWindow = nWindowedMode == 2;
        WindowedModeWrapper::desiredClientWidth = Screen.Width > 0 ? Screen.Width : nWidth;
        WindowedModeWrapper::desiredClientHeight = Screen.Height > 0 ? Screen.Height : nHeight;
        return WindowedModeWrapper::CreateWindowExA_Hook(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    BOOL WINAPI ClipCursorHook(const RECT* pRect)
    {
        if (!hGameWindow)
            return ClipCursor(pRect);
        if (!pRect)
            CursorConfinement::Release();
        else
            CursorConfinement::Update(hGameWindow);
        return TRUE;
    }

    // A camera resized before it has its rasters (sub_42DEB0, from the view's WM_SIZE): the window of the windowed
    // modes gets its size before the device and the main camera's rasters exist
    SafetyHookInline shResizeCamera = {};
    int __cdecl ResizeCamera(uint8_t* pCamera, uint32_t nWidth, uint32_t nHeight)
    {
        if (pCamera && (!*(void**)(pCamera + 96) || !*(void**)(pCamera + 100)))
            return 0;
        if (dword_820B90.is_initialized() && pCamera == dword_820B90.get() && nWidth && nHeight)
        {
            Screen.Width = int(nWidth);
            Screen.Height = int(nHeight);
            UpdateScreen();
        }
        return shResizeCamera.unsafe_ccall<int>(pCamera, nWidth, nHeight);
    }

    // The 2D camera's subrasters of the main camera (sub_42CD60), from the same WM_SIZE
    SafetyHookInline shResize2DCamera = {};
    char __cdecl Resize2DCamera()
    {
        for (auto pCamera : { dword_820B90.get(), dword_81C488.get() })
        {
            if (!pCamera || !*(void**)(pCamera + 96) || !*(void**)(pCamera + 100))
                return 0;
        }
        return shResize2DCamera.unsafe_ccall<char>();
    }

    void Init()
    {
        auto exe = GetModuleHandleA(NULL);
        IATHook::Replace(exe, "USER32.DLL", std::forward_as_tuple("ClipCursor", ClipCursorHook));

        auto pattern = hook::pattern("55 8B EC 83 EC 10 53 8B 5D 08 33 C0 56 3B D8 57 0F 84 ? ? ? ? 8B 0D");
        shResizeCamera = safetyhook::create_inline(pattern.get_first(0), ResizeCamera); //0x42DEB0
        pattern = hook::pattern("55 8B EC 83 EC 20 A1 ? ? ? ? 56 33 F6 3B C6 74 ? 8B 0D");
        dword_820B90.SetAddress(*pattern.get_first<uint8_t**>(7)); //dword_820B90
        dword_81C488.SetAddress(*pattern.get_first<uint8_t**>(0x2E)); //dword_81C488
        shResize2DCamera = safetyhook::create_inline(pattern.get_first(0), Resize2DCamera); //0x42CD60

        // The window of the game: CWnd::CreateEx of sub_402890 (a jump into GOG's code cave there), its HWND at +0x1C.
        // The retail exe shows it at once, as GOG's does (window focus).
        pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 53 8B CE ? ? ? ? ? 8B 56 1C");
        static auto WindowCreated = safetyhook::create_mid(pattern.get_first(18), [](SafetyHookContext& regs)
        {
            hGameWindow = *(HWND*)(regs.esi + 0x1C);
            ColdFearMenu::Get().window = hGameWindow;
            ColdFearRuntime::TrackWindow(hGameWindow);
            if (!bGOG)
                ShowWindow(hGameWindow, SW_SHOW);
            CursorConfinement::Frame(hGameWindow);
        }); //0x402905

        if (!nWindowedMode)
            return;

        auto videoMode = hook::pattern("88 5D C8 C7 45 D4 20 00 00 00");
        auto parameters = hook::pattern("B9 0E 00 00 00 33 C0 BF ? ? ? ? F3 AB");
        auto fill = hook::pattern("8A 44 24 08 83 EC 10 53 56 57 BB 01 00 00 00 33 FF 84 C3");

        IATHook::Replace(exe, "USER32.DLL", std::forward_as_tuple("CreateWindowExA", CreateWindowExAHook));

        // RenderWare's windowed video mode instead of a full screen one (the startup's windowed byte, sub_42D750)
        static auto WindowedVideoMode = safetyhook::create_mid(videoMode.get_first(0x1B), [](SafetyHookContext& regs)
        {
            *(uint8_t*)(regs.ebp - 0x38) = 1;
        }); //0x402949

        dword_8A2620.SetAddress(*parameters.get_first<PresentParameters*>(8)); //dword_8A2620
        // The windowed branch leaves RECT.top in the register used as the pixel format.
        // Use the selected display's format for the native depth-buffer capability checks.
        auto format = hook::pattern("39 3D ? ? ? ? 74 ? 89 1D ? ? ? ? 8B C6 A3");
        static auto WindowedFormat = safetyhook::create_mid(format.get_first(), [](SafetyHookContext& regs)
        {
            if (dword_8A2620->Windowed)
                regs.esi = (*reinterpret_cast<const uint32_t**>(regs.esp + 0x20))[3];
        });
        shFillPresentParameters = safetyhook::create_inline(fill.get_first(0), FillPresentParameters); //0x6354B0
    }
}

// The stability fixes of GOG's version (v1-v3, Nov 2025), for the retail exe; GOG's has them already. Left out: the
// Russian save/date fixes (the retail game has no Russian), the logo, and the attract demo.
void PortGOGFixes()
{
    ColdFearRuntime::InstallRetailController();

    // Alt+Tab: losing the focus doesn't pause and reset the game (WM_ACTIVATEAPP, sub_4045D0); the cursor is kept in the
    // window when it's active again (on several monitors it could leave the game's)
    auto pattern = hook::pattern("84 C0 74 05 E8 ? ? ? ? 8B 45 08 57 53 56 50 C6 05");
    auto activate = pattern.get_first<uint8_t>();
    bActive.SetAddress(*reinterpret_cast<uint8_t**>(activate + 18));
    injector::WriteMemory<uint8_t>(activate + 2, 0xEB, true); //0x402843
    struct ActivateHook
    {
        void operator()(injector::reg_pack& regs)
        {
            bActive = 1;
            if (regs.ebx) // wParam: activated
            {
                RECT rect{};
                Windowed::ClipCursorHook(&rect);
            }
        }
    }; injector::MakeInline<ActivateHook>(activate + 16, activate + 23); //0x402851, 0x402858

    // Frame rate: the game waited to keep a frame at least 1/120 s; it follows high refresh rates instead
    injector::WriteMemory<uint8_t>(hook::pattern("F6 C4 05 7A 0C C7 05 ? ? ? ? 89 88 08 3C").get_first<uint8_t>(3), 0xEB, true); //0x4044CB

    // An invalid blend operation (D3DRS_BLENDOP -1) that modern drivers reject: the default, add
    pattern = hook::pattern("8B 44 24 04 8B 4C 24 08 39 0C C5 ? ? ? ? 74");
    static auto BlendOp = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
    {
        if (regs.eax == D3DRS_BLENDOP && regs.ecx == 0xFFFFFFFF)
            regs.ecx = D3DBLENDOP_ADD;
    }); //0x639818

    // Crashes on a missing object: two functions that walk a list given to them check it first
    pattern = hook::pattern("53 8B 5C 24 08 56 8B 73 14 85 F6 74 1B 57 8B 7C 24 14");
    static auto pReturn1 = pattern.get_first<uint8_t>(0x28); //0x645018: mov eax, ebx; pop esi; pop ebx; ret
    static auto NullCheck1 = safetyhook::create_mid(pattern.get_first(0x0D), [](SafetyHookContext& regs)
    {
        if (*(uint32_t*)(regs.esp + 0x10) == 0)
            regs.eip = uintptr_t(pReturn1);
    }); //0x644FFD
    pattern = hook::pattern("A1 ? ? ? ? 83 EC 08 53 56 57 8B 7C 24 1C 33 F6 8D 1C 38");
    static auto pReturn2 = pattern.get_first<uint8_t>(0x353); //0x6498D3: pop edi; pop esi; mov eax, 1; ...
    static auto NullCheck2 = safetyhook::create_mid(pattern.get_first(0x0F), [](SafetyHookContext& regs)
    {
        if (regs.edi == 0)
            regs.eip = uintptr_t(pReturn2);
    }); //0x64958F

    // A constructor that left five members uninitialized
    pattern = hook::pattern("8D 4E 28 33 C0 C7 06 ? ? ? ? 89 11");
    static auto ZeroMembers = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
    {
        std::memset((void*)regs.esi, 0, 5 * sizeof(uint32_t));
    }); //0x43F2E3

    // Wrong members used (GOG's register and offset corrections)
    injector::WriteMemory<uint8_t>(hook::pattern("8B 0D ? ? ? ? 8B 42 64 8B 51 64 50 52 E8").get_first<uint8_t>(7), 0x41, true); //0x42AFE0: mov eax, [ecx+64h]
    injector::WriteMemory<uint8_t>(hook::pattern("89 78 0C 89 70 10 8B 43 64 89 78 0C 89 70 10").get_first<uint8_t>(8), 0x60, true); //0x42DF2D: mov eax, [ebx+60h]
    injector::WriteMemory<uint8_t>(hook::pattern("89 58 FC 89 10 83 C0 08 49 75 F5 4F").get_first<uint8_t>(1), 0x50, true); //0x63A144: mov [eax-4], edx
    injector::WriteMemory<uint8_t>(hook::pattern("89 68 FC 89 18 83 C0 08 49 75 F5 4E").get_first<uint8_t>(1), 0x50, true); //0x63A7A4: mov [eax-4], edx

    // Objects outside the view aren't left out, the shadows they cast into it stay (GOG v3)
    injector::WriteMemory<uint16_t>(hook::pattern("25 00 41 00 00 74 23 D9 42 0C D9 E0").get_first<uint16_t>(5), 0x9090, true); //0x62BA2C

    // Screen Configuration: the brightness and contrast bars show their values (the game put them into other items)
    pattern = hook::pattern("DB 45 E0 D9 9B EC 04 00 00 E8");
    static auto Brightness = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
    {
        *(float*)(regs.ebx + 0x277C) = *(float*)(regs.ebx + 0x4EC);
    }); //0x409A95
    pattern = hook::pattern("DB 45 E0 D9 9B DC 09 00 00 83 BB");
    static auto Contrast = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
    {
        *(float*)(regs.ebx + 0x2C6C) = *(float*)(regs.ebx + 0x9DC);
    }); //0x409AA6

    // Videos: the window's messages are handled while one plays (the title music and the window didn't respond)
    pattern = hook::pattern("6A 05 B9 ? ? ? ? E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 56");
    static auto VideoMessages = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs)
    {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }); //0x486E70
}

int __cdecl sub_42B460(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, int a9, int a10, int a11)
{
    auto _sub_42B460 = (int(__cdecl*)(float, float, float, float, float, float, float, float, int, int, int)) sub_42B460_addr;

    uint32_t n_offsetX1 = static_cast<uint32_t>((a1));
    uint32_t n_offsetY1 = static_cast<uint32_t>((a2));
    uint32_t n_offsetX2 = static_cast<uint32_t>((a3));
    uint32_t n_offsetY2 = static_cast<uint32_t>((a4));
    auto Color = *(FColor*)&a9;

    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("{0:d} {1:d} {2:d} {3:d} {4:08x} {5:08x} {6:08x}", n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, a9, a10, a11); });

    a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
    a3 = (a3 / Screen.fHudScaleX) + Screen.fHudOffset;

    if ((n_offsetX1 >= 433 && n_offsetX1 <= 606) && (n_offsetX2 >= 441 && n_offsetX2 <= 616) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && ((Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) || (Color.R == 0xfe && Color.G == 0xfe && Color.B == 0xfe) || (Color.A == 0xff && Color.R == 0xff)))
    {
        a1 += fWidescreenHudOffset;
        a3 += fWidescreenHudOffset;
    }

    if (
        (n_offsetX1 == 24) && (n_offsetX2 == 136) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && (Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) || //text (RESISTANCE | LIFE)
        (n_offsetX1 == 92) && (n_offsetX2 == 204) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && (Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) //text (INTERACTION)
        )
    {
        a1 -= fWidescreenHudOffset;
        a3 -= fWidescreenHudOffset;
    }

    return _sub_42B460(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

void __cdecl sub_42B610(float a1, float a2, float a3, float a4, int a5, int a6, int a7)
{
    auto _sub_42B610 = (void(__cdecl*)(float, float, float, float, int, int, int)) sub_42B610_addr;

    uint32_t n_offsetX1 = static_cast<uint32_t>((a1));
    uint32_t n_offsetY1 = static_cast<uint32_t>((a2));
    uint32_t n_offsetX2 = static_cast<uint32_t>((a3));
    uint32_t n_offsetY2 = static_cast<uint32_t>((a4));

    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("{0:d} {1:d} {2:d} {3:d} {4:08x} {5:08x} {6:08x}", n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, a5, a6, a7); });

    if (a1 != 0.0f && a2 != 0.0f) //menu background rendered here
    {
        a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
        a3 = (a3 / Screen.fHudScaleX);
    }

    if (((n_offsetX1 == 21) && (n_offsetX2 >= 0 && n_offsetX2 <= 166) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && (a6 == 0 && a7 == 0))) //health, resistance bar etc
    {
        a1 -= fWidescreenHudOffset;
        //a3 -= fWidescreenHudOffset;
    }

    return _sub_42B610(a1, a2, a3, a4, a5, a6, a7);
}

void __cdecl sub_42B740(float a1, float a2, float a3, float a4, int a5, int a6, int a7) //health bar border, damage red bar etc
{
    auto _sub_42B740 = (void(__cdecl*)(float, float, float, float, int, int, int)) sub_42B740_addr;
    auto Color = *(FColor*)&a5;

    uint32_t n_offsetX1 = static_cast<uint32_t>((a1));
    uint32_t n_offsetY1 = static_cast<uint32_t>((a2));
    uint32_t n_offsetX2 = static_cast<uint32_t>((a3));
    uint32_t n_offsetY2 = static_cast<uint32_t>((a4));

    // The retail frontend draws its own black top and bottom strips with no cutscene blend.
    // They span the whole 640-wide canvas and must not receive the centered HUD transform.
    if (CutsceneBlend.get() <= 0.0f && a1 == 0.0f && a3 == 640.0f && a4 > 0.0f && a4 <= 100.0f &&
        (a2 == 0.0f || (a2 >= 380.0f && a2 + a4 == 480.0f)) && a5 == int(0xff000000) && a6 == 0 && a7 == 0)
        return _sub_42B740(a1, a2, a3, a4, a5, a6, a7);

    if (CutsceneBlend.get() > 0.0f && a1 == 0.0f && ((a2 >= 0.0f && a2 <= 69.5f) || (a2 >= 400.0f && a2 <= 480.0f)) && a3 == 640.0f && a5 == 0xff000000 && a6 == 0 && a7 == 0) //borders
    {
        const float blend = std::clamp(CutsceneBlend.get(), 0.0f, 1.0f);
        const float animation = AnimatedCutsceneBorders ? blend : 1.0f;
        if (!AnimatedCutsceneBorders && blend > 0.0f)
        {
            a4 /= blend;
            if (a2 > 0.0f)
                a2 = 480.0f - a4;
        }
        // Pillarbox: the picture at the cutscenes' 4:3, drawn with the top bar (once a frame); the canvas is 640 wide
        if ((nCutsceneBorders & 2) && a2 < 69.5f && Screen.fAspectRatio > 4.0f / 3.0f)
        {
            float side = (640.0f - 640.0f * (4.0f / 3.0f) / Screen.fAspectRatio) / 2.0f * animation;
            _sub_42B740(0.0f, 0.0f, side, 480.0f, a5, a6, a7);
            _sub_42B740(640.0f - side, 0.0f, side, 480.0f, a5, a6, a7);
        }
        if (nCutsceneBorders & 1)
            _sub_42B740(a1, a2, a3, a4, a5, a6, a7);
        return;
    }

    if (a1 == 0.0f && a2 == 0.0f && a3 == Screen.fWidth && a4 == Screen.fHeight && ((Color.R == 0 && Color.G == 0 && Color.B == 0) || (Color.R == 0xff && Color.G == 0xff && Color.B == 0xff)) && a6 == 0 && a7 == 0x00000001) //fading
        return _sub_42B740(a1, a2, a3, a4, a5, a6, a7);

    a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
    a3 = (a3 / Screen.fHudScaleX);

    if (((n_offsetX1 == 20 || n_offsetX1 == 164) && (n_offsetX2 == 1 || n_offsetX2 == 144 || n_offsetX2 == 145) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && (Color.R == 0 && Color.G == 0 && Color.B == 0) && (a6 == 0))//health bar border etc
        || ((n_offsetX1 >= 20 && n_offsetX1 <= 166) && (Color.R == 0xff && Color.G == 0x24 && Color.B == 0x08)))
    {
        a1 -= fWidescreenHudOffset;
        //a3 -= fWidescreenHudOffset;
    }

    return _sub_42B740(a1, a2, a3, a4, a5, a6, a7);
}

void __cdecl sub_42BA20(float a1, float a2, int a3, int a4, int a5)
{
    auto _sub_42BA20 = (void(__cdecl*)(float, float, int, int, int)) sub_42BA20_addr;

    a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
    //a3 = (a3 / Screen.fHudScaleX) + Screen.fHudOffset;

    return _sub_42BA20(a1, a2, a3, a4, a5);
}

int __cdecl sub_42BBA0(int a1, int a2, int a3, int a4, char a5) // all hud borders and other things
{
    auto _sub_42BBA0 = (int(__cdecl*)(int, int, int, int, char)) sub_42BBA0_addr;

    *(float*)a1 = (*(float*)a1 / Screen.fHudScaleX) + Screen.fHudOffset;
    *(float*)a2 = (*(float*)a2 / Screen.fHudScaleX) + Screen.fHudOffset;

    return _sub_42BBA0(a1, a2, a3, a4, a5);
}

int __cdecl sub_42C8D0(float a1, float a2, int a3, char a4, char a5, char a6)
{
    auto _sub_42C8D0 = (int(__cdecl*)(float, float, int, char, char, char)) sub_42C8D0_addr;

    a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;

    return _sub_42C8D0(a1, a2, a3, a4, a5, a6);
}

int __cdecl sub_42CA50(float a1, float a2, float a3, float a4, int a5, char a6, char a7, char a8)
{
    auto _sub_42CA50 = (int(__cdecl*)(float, float, float, float, int, char, char, char)) sub_42CA50_addr;

    a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
    a3 = (a3 / Screen.fHudScaleX) + Screen.fHudOffset;

    return _sub_42CA50(a1, a2, a3, a4, a5, a6, a7, a8);
}

void __cdecl sub_42B900(float a1, float a2, float a3, float a4, int a5)
{
    auto draw = reinterpret_cast<void(__cdecl*)(float, float, float, float, int)>(sub_42B900_addr);
    auto clear = reinterpret_cast<void(__cdecl*)(float, float, float, float, int, int, int)>(sub_42B740_addr);
    clear(0.0f, 0.0f, 640.0f, 480.0f, 0xFF000000, 0, 0);
    const auto frame = ColdFearMovie::Layout(Screen.Width, Screen.Height, ColdFearMovie::Current, nFMVWidescreenMode);
    if (frame.width > 0.0f && frame.height > 0.0f)
        return draw(frame.x, frame.y, frame.width, frame.height, a5);
    a1 = (a1 / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.0f));
    a3 /= Screen.fHudScaleX;
    return draw(a1, a2, a3, a4, a5);
}

int __cdecl sub_42BD11(float a1, float a2, int a3, char a4, unsigned __int8 a5)
{
    return 0; //?
}

// The camera's projection (sub_62BB30) for the shadow camera (sub_44CE70): without the widescreen field of view, as
// GOG's version keeps it (v3, "slightly misplaced character shadows"); sub_62D480 updates the camera from it
uintptr_t sub_62D480_addr;
int __cdecl _sub_62BB30(int a1, float* a2)
{
    static auto sub_62D480 = (int(__cdecl*)(int)) sub_62D480_addr;

    *(float*)(a1 + 104) = a2[0];
    *(float*)(a1 + 108) = a2[1];
    *(float*)(a1 + 112) = 1.0f / *(float*)(a1 + 104);
    *(float*)(a1 + 116) = 1.0f / *(float*)(a1 + 108);
    if (*(uint32_t*)(a1 + 4))
        sub_62D480(*(uint32_t*)(a1 + 4));
    return a1;
}

// GOG's 2D canvas, back to the game's 640x480 at a scale of 1: the HUD functions above work in it
auto fCanvasWidth = ([]() -> float* { auto p = hook::pattern("C7 45 F0 00 00 ? 44 C7 45 F4 00 00 F0 43"); return p.get_first<float>(3); })(); //0x403A71
auto fCanvasScaleX = ([]() -> float* { auto p = hook::pattern("8D 4D F0 50 51 C7 45 F8"); return p.get_first<float>(8); })(); //0x42AEAA
auto fCanvasScaleY = ([]() -> float* { auto p = hook::pattern("8D 4D F0 50 51 C7 45 F8"); return p.get_first<float>(15); })(); //0x42AEB1

// The menu's page names (off_762C00): the fix's page takes Language_EUROPE's slot, see Menu.hpp
const bool menuPageBound = []() { auto p = hook::pattern("8B 0C BD ? ? ? ? 8B F0 8A 19"); szMenuPageName.SetAddress(*p.get_first<const char**>(3) + ColdFearMenu::PageSlot); return true; }(); //0x762C04

namespace Menu
{
    int (__thiscall* SetPage)(void*, int, char) = nullptr;
    int (__thiscall* PageShown)(void*, int, int) = nullptr;
    // MenuPC.cfg as the parser reads it
    SafetyHookInline shParseCfg = {};
    char __fastcall ParseCfg(void* pParser, void* edx, const char* szText, int nLength)
    {
        auto szMenu = ColdFearMenu::AddPage(szText);
        return shParseCfg.unsafe_fastcall<char>(pParser, edx, szMenu, szMenu == szText ? nLength : int(std::strlen(szMenu)));
    }

    // A page's frame
    SafetyHookInline shUpdatePage = {};
    char __fastcall UpdatePage(uint8_t* pPage, void* edx, float fTime)
    {
        ColdFearMenu::Update(pPage);
        const auto result = shUpdatePage.unsafe_fastcall<char>(pPage, edx, fTime);
        ColdFearMenu::Describe(pPage);
        return result;
    }

    // Enter on the item selected
    SafetyHookInline shActivate = {};
    bool __fastcall Activate(uint8_t* pPage, void* edx)
    {
        if (ColdFearMenu::Use(pPage, 0))
            return false;
        return shActivate.unsafe_fastcall<bool>(pPage, edx);
    }

    SafetyHookInline shLeft = {};
    void __fastcall Left(uint8_t* pPage, void* edx)
    {
        if (!ColdFearMenu::Use(pPage, -1))
            shLeft.unsafe_fastcall(pPage, edx);
    }

    SafetyHookInline shRight = {};
    void __fastcall Right(uint8_t* pPage, void* edx)
    {
        if (!ColdFearMenu::Use(pPage, 1))
            shRight.unsafe_fastcall(pPage, edx);
    }

    SafetyHookInline shCommitPage = {};
    int __fastcall CommitPage(uint8_t* menu, void* edx)
    {
        // Slot 1 hosts Plugins; its former language-page commit would unload the font bank.
        if (*reinterpret_cast<int*>(menu + 0xC69C0) == ColdFearMenu::PageSlot)
            return *reinterpret_cast<int*>(menu + 0xC69BC);
        return shCommitPage.unsafe_fastcall<int>(menu, edx);
    }

    SafetyHookInline shFrontend = {};
    int __fastcall Frontend(uint8_t* menu, void* edx, float time)
    {
        ColdFearMenu::Get().frontend = true;
        const auto state = shFrontend.unsafe_fastcall<int>(menu, edx, time);
        if (state == 2)
            ColdFearMenu::Get().frontend = false;
        return state;
    }

    void Init()
    {
        HMODULE hModule = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&Init, &hModule);

        auto sounds = hook::pattern("89 81 A8 C7 00 00 6A 00 B9 ? ? ? ? E8");
        auto texts = hook::pattern("8B F8 85 FF 0F 8C ? ? ? ? 68 ? ? ? ? B9");
        auto parse = hook::pattern("83 C3 FC 8D 47 04 53 50 E8");
        auto update = hook::pattern("55 8B EC 83 EC 34 53 56 57 8B D9 C6 45 F0 00 C6 45 FE 01");
        auto activate = hook::pattern("55 8B EC 81 EC 0C 01 00 00 53 8B D9 56 57 83 BB 94 C5 00 00 0C");
        auto left = hook::pattern("56 8B F1 8B 86 8C C5 00 00 85 C0 7D 05 E8 ? ? ? ? 8B 86 8C C5 00 00 8D 0C 80 C1 E1 04 2B C8 C1 E1 04 8B 44 31 20 83 E8 04 74 ? 83 E8 02 8B CE 74 ? 6A 02");
        auto right = hook::pattern("56 8B F1 8B 86 8C C5 00 00 85 C0 7D 05 E8 ? ? ? ? 8B 86 8C C5 00 00 8D 0C 80 C1 E1 04 2B C8 C1 E1 04 8B 44 31 20 83 E8 04 74 ? 83 E8 02 8B CE 74 ? 6A 01");
        auto start = hook::pattern("8B 8B 00 D4 00 00 6A 01 6A 04 E8 ? ? ? ? 8B 83 94 C5 00 00 8B 8B 00 D4 00 00 50 6A 04 E8");
        auto commit = hook::pattern("55 8B EC 51 53 8B D9 56 57 8B B3 C0 69 0C 00 83 FE 0D 89 75 FC");
        auto frontend = hook::pattern("55 8B EC 81 EC 88 01 00 00 53 8B D9 8B 43 04 89 45 E8 A0 ? ? ? ? 84 C0 74 0C B8 02 00 00 00");

        // GOG's page code takes the menu from its caller's saved EBX, which the page hook replaces: the page's own menu
        // pointer instead
        if (bGOG)
        {
            auto gogMenu = hook::pattern("8B 4C E4 08 8B 89 BC 69 0C 00 83 F9 03");
            struct GOGMenuHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto menu = *reinterpret_cast<uint8_t**>(regs.ebx + 0xD400);
                    regs.ecx = *reinterpret_cast<uint32_t*>(menu + 0xC69BC);
                }
            }; injector::MakeInline<GOGMenuHook>(gogMenu.get_first(0), gogMenu.get_first(10));
        }
        SetPage = reinterpret_cast<decltype(SetPage)>(injector::GetBranchDestination(start.get_first(10), true).as_int());
        PageShown = reinterpret_cast<decltype(PageShown)>(injector::GetBranchDestination(start.get_first(30), true).as_int());
        ColdFearMenu::Get().skipStart = [](uint8_t* page)
        {
            if (auto owner = *reinterpret_cast<void**>(page + 0xD400))
            {
                SetPage(owner, 4, 1);
                PageShown(owner, 4, 3);
            }
        };

        // The menu's sounds (sub_495590, 0: move, 1: back, 2: select)
        auto playSound = injector::GetBranchDestination(sounds.get_first(13), true).get<void>();
        auto sound = *sounds.get_first<void*>(9);
        // The texts of menu.lng (dword_876918, sub_461730 looks a key up in it)
        ColdFearMenu::Init(hModule, playSound, sound, *texts.get_first<void*>(16));

        injector::WriteMemory<const char*>(szMenuPageName.get_ptr(), ColdFearMenu::PageName, true);

        shParseCfg = safetyhook::create_inline(injector::GetBranchDestination(parse.get_first(8), true).get<void>(), ParseCfg); //0x485E10
        shUpdatePage = safetyhook::create_inline(update.get_first(0), UpdatePage); //0x40AC40
        shActivate = safetyhook::create_inline(activate.get_first(0), Activate); //0x40C690
        shLeft = safetyhook::create_inline(left.get_first(), Left); //0x40C360
        shRight = safetyhook::create_inline(right.get_first(), Right); //0x40C490
        shCommitPage = safetyhook::create_inline(commit.get_first(), CommitPage); //0x40F680
        shFrontend = safetyhook::create_inline(frontend.get_first(), Frontend); //0x40E6D0
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            // The display's real size on scaled desktops (the exe isn't DPI aware)
            SetProcessDPIAware();

            // The publisher, Darkworks and RenderWare logos of the startup
            if (FixSettings().Bool("MAIN.SkipIntro"))
            {
                auto logos = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 0C");
                injector::MakeNOP(logos.get_first(5), 5, true);
                injector::MakeNOP(logos.get_first(15), 5, true);
                injector::MakeNOP(logos.get_first(25), 5, true);
            }

            // The frame's end (RwCameraShowRaster of the main camera) keeps the cursor in the window
            {
                auto show = hook::pattern("8B 15 ? ? ? ? 0F 94 C0 50 51 52 E8");
                static auto ShowRaster = safetyhook::create_mid(show.get_first(12), [](SafetyHookContext& regs)
                {
                    static std::once_flag once;
                    std::call_once(once, []() { WFP::onGameInitEvent().executeAll(); });
                    WFP::onGameProcessEvent().executeAll();
                    CursorConfinement::Frame(Windowed::hGameWindow);
                }); //0x42DE4E
            }

            ColdFearMouse::Init();
            ColdFearMovie::Init();
            auto cutscene = hook::pattern("56 B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 84 C0 5E");
            CutsceneBlend.SetAddress(*cutscene.get_first<uintptr_t>(2) + 44);
            Resolution::Init();
            Windowed::Init();
            Menu::Init();
            ColdFearRuntime::Init(bGOG);

            if (bGOG)
            {
                injector::WriteMemory<float>(fCanvasWidth, 640.0f, true);
                injector::WriteMemory<float>(fCanvasScaleX, 1.0f, true);
                injector::WriteMemory<float>(fCanvasScaleY, 1.0f, true);
            }
            else
            {
                PortGOGFixes();
            }

            //Hud
            sub_42B380_addr = (uintptr_t)hook::pattern("55 8B EC 8B 45 08 D9 05 ? ? ? ? D8 08").get_first<uint32_t>(0);
            sub_42B3A0_addr = (uintptr_t)hook::pattern("55 8B EC 8A 45 14 56 84 C0 74 0D D9 45 0C E8 ? ? ? ? D9 45 10").get_first<uint32_t>(0);
            sub_42B460_addr = (uintptr_t)hook::pattern("55 8B EC A0 ? ? ? ? 56 84 C0 0F 84 ? ? 00 00").get_first<uint32_t>(0);
            sub_42B610_addr = (uintptr_t)hook::pattern("55 8B EC A0 ? ? ? ? 57 84 C0 0F 84 ? ? ? ? 8B 3D ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 03 C8 8D 57 34").get_first();
            sub_42B740_addr = (uintptr_t)hook::pattern("55 8B EC A0 ? ? ? ? 57 84 C0 0F 84 ? ? ? ? 8B 3D ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 03 C8 8D 57 28").get_first();
            sub_42BA20_addr = (uintptr_t)hook::pattern("55 8B EC 56 57 8B 3D ? ? ? ? 85 FF 0F 84 C8 00 00 00").get_first<uint32_t>(0);
            sub_42BBA0_addr = (uintptr_t)hook::pattern("55 8B EC 51 A1 ? ? ? ? 8B 0D ? ? ? ? 56").get_first<uint32_t>(0);
            //sub_42BD11_addr = hook::pattern("").count(1).get(0).get<uint32_t>(0); ?
            sub_42C880_addr = (uintptr_t)hook::pattern("55 8B EC 8B 45 08 8B 0D ? ? ? ? 8B 15 ? ? ? ? 89 08").get_first<uint32_t>(0);
            sub_42C8D0_addr = (uintptr_t)hook::pattern("55 8B EC 83 EC 0C 8A 55 14 53 56 33 F6 84 D2").get_first<uint32_t>(0);
            sub_42CA50_addr = (uintptr_t)hook::pattern("55 8B EC 83 EC 0C 8A 55 1C 53 56 33 F6 84 D2").get_first<uint32_t>(0);
            sub_42B900_addr = (uintptr_t)hook::pattern("55 8B EC A1 ? ? ? ? 8B 0D ? ? ? ? 56 8B 35 ? ? ? ? 57 03 C8").get_first<uint32_t>(0);

            auto pattern = hook::pattern("E8 ? ? ? ?");
            for (size_t i = 0; i < pattern.size(); ++i)
            {
                auto addr = pattern.get(i).get<uint32_t>(0);
                auto dest = injector::GetBranchDestination(addr, true).as_int();

                if (dest == sub_42B460_addr)
                    injector::MakeCALL(addr, sub_42B460, true);

                if (dest == sub_42B610_addr)
                    injector::MakeCALL(addr, sub_42B610, true);

                if (dest == sub_42B740_addr)
                    injector::MakeCALL(addr, sub_42B740, true);

                if (dest == sub_42BA20_addr)
                    injector::MakeCALL(addr, sub_42BA20, true);

                if (dest == sub_42C8D0_addr)
                    injector::MakeCALL(addr, sub_42C8D0, true);

                if (dest == sub_42CA50_addr)
                    injector::MakeCALL(addr, sub_42CA50, true);

                ///if (dest == sub_42B380_addr)
                ///	injector::MakeCALL(addr, sub_42B380, true);

                ///if (dest == sub_42B3A0_addr)
                ///	injector::MakeCALL(addr, sub_42B3A0, true);

                //if (dest == sub_42BBA0_addr)
                //	injector::MakeCALL(addr, sub_42BBA0, true);

                //if (dest == sub_42BD11_addr)
                //	injector::MakeCALL(addr, sub_42BD11, true);

                //if (dest == sub_42C880_addr)
                //	injector::MakeCALL(addr, sub_42C880, true);

            }

            pattern = hook::pattern("89 45 18 8B 5D FC DB 45 18 D8 0D");
            struct sub_42BBA0Hook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uintptr_t*)(regs.ebp + 0x18) = regs.eax;
                    regs.ebx = *(uintptr_t*)(regs.ebp - 0x4);

                    *(float*)(regs.esi + 0x10) = ((*(float*)(regs.esi + 0x10)) / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.0f));
                    *(float*)(regs.esi + 0x18) = ((*(float*)(regs.esi + 0x18)) / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.0f));
                }
            }; injector::MakeInline<sub_42BBA0Hook>(pattern.get_first<uint32_t>(0), pattern.get_first<uint32_t>(6)); //0x42BC7B, 0x42BC7B + 6

            //FMV
            pattern = hook::pattern("D9 5C 24 04 DB 45 10 D9 1C 24 6A 00 6A 00 E8 ? ? ? ? 83 C4 14");
            injector::MakeCALL(pattern.get_first<uint32_t>(14), sub_42B900, true); //0x486EC9

            //FOV: both scales of the camera's projection (sub_62BB30), GOG's fixed 16:9 one included. The first match is the
            //original function; GOG's code cave keeps a copy of it for the shadow camera.
            pattern = hook::pattern("D9 05 ? ? ? ? 89 4E 68 8B 50 04");
            auto sub_62BB30_addr = pattern.get_first<uint8_t>(-0xB); //0x62BB30
            static auto projection = safetyhook::create_mid(sub_62BB30_addr, [](SafetyHookContext&)
            {
                const float factor = ColdFearMenu::Get().frontend || CutsceneBlend.get() > 0.0f ? 1.0f : fFOVFactor;
                Screen.fDynamicScreenFieldOfViewScale = 0.75f * (4.0f / 3.0f) / Screen.fAspectRatio / factor;
            });
            injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fDynamicScreenFieldOfViewScale, true);
            pattern = hook::pattern("D9 05 ? ? ? ? D8 76 6C D9 5E 74");
            injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fDynamicScreenFieldOfViewScale, true);
            sub_62D480_addr = injector::GetBranchDestination(sub_62BB30_addr + 0x34, true).as_int(); //0x62BB64: call sub_62D480

            //Shadow camera (sub_44CE70): GOG's version calls its own copy already
            if (!bGOG)
            {
                pattern = hook::pattern("89 55 FC E8 ? ? ? ? 83 C4 14");
                injector::MakeCALL(pattern.get_first(3), _sub_62BB30, true); //0x44CEDB
            }

            //pattern = hook::pattern("8D 4D 9C 8D 55 DC 51 52 8B CE");
            //struct FOVHook
            //{
            //    void operator()(injector::reg_pack& regs)
            //    {
            //        regs.ecx = regs.ebp - 0x64;
            //        regs.edx = regs.ebp - 0x24;
            //
            //        *(float *)(regs.esi + 0x40) *= Screen.fDynamicScreenFieldOfViewScale; //CAMERA_FOLLOW_MINDIST
            //        *(float *)(regs.esi + 0x44) *= Screen.fDynamicScreenFieldOfViewScale; //CAMERA_FOLLOW_MAXDIST
            //        *(float *)(regs.esi + 0xBC) *= Screen.fDynamicScreenFieldOfViewScale; //CAMERA_AIM_FOV
            //        *(float *)(regs.esi + 0xC4) *= Screen.fDynamicScreenFieldOfViewScale; //CAMERA_CONSTRAINT_FOV
            //
            //    }
            //}; //injector::MakeInline<FOVHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //0x4AF5EE, 0x4AF5EE + 6

            //injector::MakeNOP(0x402232, 2, true); //debug menu
        };
    }
} Misc;

