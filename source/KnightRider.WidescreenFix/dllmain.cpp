#include "stdafx.h"
#include "pcsx2/pcsx2.h"
#include <chrono>

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fFOVFactor;
} Screen;

HWND WINAPI CreateWindowExAHook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hWnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
    SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    return hWnd;
}

BOOL WINAPI AdjustWindowRectHook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
    return AdjustWindowRect(lpRect, dwStyle & ~WS_OVERLAPPEDWINDOW, bMenu);
}

void Init()
{
    CIniReader iniReader("");
    Screen.nWidth = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.nHeight = iniReader.ReadInteger("MAIN", "ResY", 0);

    if (!Screen.nWidth || !Screen.nHeight)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //Default res
    auto pattern = hook::pattern("68 E0 01 00 00 8D 44 24 1C 68 80 02 00 00 8D 4C 24 1C 50 51 6A 03 8B CE");
    injector::WriteMemory(pattern.get_first(1), Screen.nHeight, true);
    injector::WriteMemory(pattern.get_first(10), Screen.nWidth, true);

    //Menu res
    pattern = hook::pattern("8B 44 24 28 89 30 8B 55 04 8B 6D 08");
    struct ForceResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esp + 0x24);
            *(uint32_t*)(regs.eax) = Screen.nWidth;
            regs.eax = *(uint32_t*)(regs.esp + 0x28);
            *(uint32_t*)(regs.eax) = Screen.nHeight;
        }
    }; injector::MakeInline<ForceResHook>(pattern.get_first(0), pattern.get_first(6));

    //Aspect Ratio
    pattern = hook::pattern("AB AA AA 3F"); //1.3333334
    for (size_t i = 0; i < pattern.size(); i++)
    {
        injector::WriteMemory(pattern.get(i).get<void>(0), Screen.fAspectRatio, true);
    }

    pattern = hook::pattern("A1 ? ? ? ? 83 C4 0C D8 C9 50 51");
    injector::WriteMemory<float>(*pattern.get_first<void*>(1), Screen.fAspectRatio, true);

    //2D
    static auto dword_56F775 = hook::get_pattern("8B 8E B0 01 00 00 8B 01 FF 90 B4 00 00 00 8B 8E 7C 01 00 00 85 C9", 0);
    static auto dword_4AF6E1 = hook::get_pattern("85 F6 74 11 8B 46 04 48 89 46 04 75 08 8B 06 6A 01 8B CE FF 10 5E C2 08 00", 0);
    static auto dword_40AC69 = hook::get_pattern("8B 4E 2C E8 ? ? ? ? 8B 4E 1C E8 ? ? ? ? 8B 4E 20 E8 ? ? ? ? 8A 46 58 84 C0", 0);
    static auto dword_40AC71 = hook::get_pattern("8B 4E 1C E8 ? ? ? ? 8B 4E 20 E8 ? ? ? ? 8A 46 58 84 C0 74 10 8B 46 14 8B 4E", 0);
    static auto dword_4BD9A5 = hook::get_pattern("8B 96 C4 00 00 00 33 C9 8B 52 08 8B 82 EC 00 00 00 85 C0", 0);
    static auto dword_4081EF = hook::pattern("83 7C 24 08 01 75 08 8B 49 10 E8 ? ? ? ? C2 08 00").count(2).get(1).get<void>(15); //objective marker
    static auto dword_404D92 = hook::get_pattern("8B 4B 20 E8 ? ? ? ? 5F 5E", 0); //night vision 1
    static auto dword_404D9A = hook::get_pattern("5F 5E 5B C2 08 00 81 EC C0 00 00 00", 0); //night vision 2
    static auto dword_4B514B = hook::get_pattern("C6 86 B4 00 00 00 00 8B 43 0C 8B 48 4C 89 8E B8 00 00 00", 0);
    static auto dword_40ACAE = hook::get_pattern("6A 04 E8 ? ? ? ? 8B 4E 28 E8 ? ? ? ? 5E C2 08 00", 15); //dialog faces

    pattern = hook::pattern("C7 44 24 14 00 00 00 00 DA 4C 24 10 E8 ? ? ? ? 89 44 24 7C");
    struct HUDHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 0x14) = 0;

            auto ret1 = *(void**)(regs.esp + 0xC8);
            auto ret2 = *(void**)(regs.esp + 0xF0);
            auto ret3 = *(void**)(regs.esp + 0xF4);

            if (ret1 == dword_56F775 && ret2 != dword_4AF6E1 && ret2 != dword_40AC69 && ret2 != dword_40AC71 && ret2 != dword_4BD9A5 && ret2 != dword_4081EF && ret2 != dword_404D92 && ret2 != dword_404D9A && ret3 != dword_4B514B)
            {
                *(int32_t*)(regs.esp + 0x70) = (int32_t)(((((Screen.fWidth * (float)(*(int32_t*)(regs.esp + 0x70))) / Screen.fHudScale) + Screen.fHudOffsetReal) / Screen.fWidth) + Screen.fHudOffsetReal);
                *(int32_t*)(regs.esp + 0x78) = (int32_t)((float)(*(int32_t*)(regs.esp + 0x78)) / Screen.fHudScale);

                if (ret2 == dword_40ACAE)
                {
                    *(int32_t*)(regs.esp + 0x70) = 20;
                }
            }
        }
    }; injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(8)); //0x5C0EB1

    pattern = hook::pattern("D8 0D ? ? ? ? 53 8B D9 56 57 D9 F2 8B 53 08");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float t = 0.0087266462f;
            float f = 0.0f;
            _asm {fstp[f]}
            f = AdjustFOV(f, Screen.fAspectRatio);
            _asm {fld[f]}
            _asm {fmul[t]}
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6)); //0x485717

    //Window style fix
    pattern = hook::pattern("8B 2D ? ? ? ? 50 68 ? ? ? ? 57 FF D5");
    injector::WriteMemory(*pattern.get_first<uint32_t*>(2), CreateWindowExAHook, true); //60E178
    pattern = hook::pattern("FF 15 ? ? ? ? A1 ? ? ? ? BB 00 00 00 40 8A 48 24 84 C9");
    injector::WriteMemory(*pattern.get_first<uint32_t*>(2), AdjustWindowRectHook, true); //60E184
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("68 E0 01 00 00 8D 44 24 1C 68 80 02"));

            if (!PCSX2::pcsx2_crc_pattern.empty())
            {
                void PCSX2Thread();
                std::thread t(PCSX2Thread);
                t.detach();
            }
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}


///////////////////////////////////////////////////////////////////////
//////////////////////Experimental PCSX2 support///////////////////////
///////////////////////////////////////////////////////////////////////

void PCSX2Thread()
{
    static auto ps2 = PCSX2({ 0x989192FE }, NULL, [](PCSX2& ps2)
        {
            Screen.nWidth = ps2.GetWindowWidth();
            Screen.nHeight = ps2.GetWindowHeight();
            Screen.fAspectRatio = ps2.GetAspectRatio();
            Screen.fWidth = static_cast<float>(Screen.nWidth);
            Screen.fHeight = static_cast<float>(Screen.nHeight);
            Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
            Screen.fHudScale = (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            float f = 80.0f;
            float t = AdjustFOV(f, Screen.fAspectRatio);
            Screen.fFOVFactor = 0.00872665f * (t / f);
        });

    while (!ps2.isCRCValid())
        std::this_thread::yield();

    ps2.vecPatches.push_back(PCSX2Memory(L"gametitle=Knight Rider: The Game (PAL-M6)(SLES-51011)"));
    ps2.vecPatches.push_back(PCSX2Memory(L"comment=Widescreen Fix by ThirteenAG https://thirteenag.github.io/wfp#kr"));
    ps2.vecPatches.push_back(PCSX2Memory(L""));
    ps2.vecPatches.push_back(PCSX2Memory(L"// Current Resolution: " + std::to_wstring(Screen.nWidth) + L"x" + std::to_wstring(Screen.nHeight) + L", Aspect Ratio: " + std::to_wstring(Screen.fAspectRatio)));
    ps2.vecPatches.push_back(PCSX2Memory(L""));

    auto ps2pattern = hook::range_pattern(ps2.EEMainMemoryStart, ps2.EEMainMemoryEnd, "AA 3F ? 3C");
    while (ps2pattern.clear().size() != 2) { std::this_thread::yield(); }
    for (size_t i = 0; i < ps2pattern.size(); i++)
    {
        ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2pattern.get(i).get<void>(0), WORD_T, LUI_ORI, &Screen.fAspectRatio, std::wstring(L"// Aspect Ratio: ") + std::to_wstring(Screen.fAspectRatio), 0x3FAAAAAB));
    }

    ps2pattern = hook::range_pattern(ps2.EEMainMemoryStart, ps2.EEMainMemoryEnd, "AB AA AA 3F");
    while (ps2pattern.clear().size() != 1) { std::this_thread::yield(); }
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2pattern.get(0).get<void>(0), WORD_T, NONE, &Screen.fAspectRatio, std::wstring(L"// Aspect Ratio: ") + std::to_wstring(Screen.fAspectRatio), 0x3FAAAAAB));

    ps2pattern = hook::range_pattern(ps2.EEMainMemoryStart, ps2.EEMainMemoryEnd, "0E 3C 02 3C 36 FA 42 34 00 00 82 44 D0 FF BD 27");
    while (ps2pattern.clear().size() != 2) { std::this_thread::yield(); }
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2pattern.get(0).get<void>(0), WORD_T, LUI_ORI, &Screen.fFOVFactor, std::wstring(L"// FOV: ") + std::to_wstring(Screen.fFOVFactor), 0x3C0EFA36));

    ps2.WritePnach();
    ps2.WriteMemoryLoop();
}