#include "stdafx.h"
#include "pcsx2/pcsx2.h"
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;
#include <ShellAPI.h>

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
    float fRadarBlipSize;
    float fRadarBlipPosition;
    float fPointSize;
    uint32_t PointSizeInstr;
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
    bool bFixFMV = iniReader.ReadInteger("MAIN", "FixFMV", 1) != 0;

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
    auto pattern = hook::pattern("68 E0 01 00 00 8D 44 24 18 68 80 02 00 00 8D 4C 24 18 50 51 6A 04");
    injector::WriteMemory(pattern.get_first(1), Screen.nHeight, true);
    injector::WriteMemory(pattern.get_first(10), Screen.nWidth, true);

    //Menu res
    pattern = hook::pattern("8B 5C 24 30 89 03 8B 45 04 8B 75 08");
    struct ForceResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = *(uint32_t*)(regs.esp + 0x2C);
            *(uint32_t*)(regs.ebx) = Screen.nWidth;
            regs.ebx = *(uint32_t*)(regs.esp + 0x30);
            *(uint32_t*)(regs.ebx) = Screen.nHeight;
        }
    }; injector::MakeInline<ForceResHook>(pattern.get_first(0), pattern.get_first(6)); //0x5E183D

    //Aspect Ratio
    pattern = hook::pattern("AB AA AA 3F"); //1.3333334
    for (size_t i = 0; i < pattern.size(); i++)
    {
        if (i != 5) //PointTGA
            injector::WriteMemory(pattern.get(i).get<void>(0), Screen.fAspectRatio, true);
    }
    pattern = hook::pattern("39 8E E3 3F"); //1.7777778
    for (size_t i = 0; i < pattern.size(); i++)
    {
        injector::WriteMemory(pattern.get(i).get<void>(0), Screen.fAspectRatio, true);
    }

    //2D
    static auto dword_7E2446 = hook::get_pattern("83 C4 0C 5D C3 55 8B CD E8 ? ? ? ? 5D C3", 13);
    static auto dword_63AFDC = hook::get_pattern("8B 44 24 04 85 C0 74 17 8B 70 04 8B C8 83 C0 04 4E", 0);
    static auto dword_65E46E = hook::get_pattern("80 88 CC 00 00 00 01 8B 41 68 8B 48 08 E8 ? ? ? ? 5E C2 08 00", 18);
    static auto dword_65E540 = hook::pattern("8B 56 64 8B 4A 08 33 D2 66 39 91 D6 00 00 00").count(2).get(1).get<void>(0);
    static auto dword_532FF8 = hook::get_pattern("8B 4E 44 E8 ? ? ? ? 8A 46 79 84 C0 0F 84 ? ? ? ? 8B 46 2C", 0);
    static auto dword_5369D3 = hook::get_pattern("83 C4 10 8D 4C 24 04 E8 ? ? ? ? 8B 4E 0C E8 ? ? ? ? 5E 59 C2 08 00", 20);
    static auto dword_52E81A = hook::get_pattern("8B 4E 30 E8 ? ? ? ? 5E C2 08 00", 8);
    static auto dword_5D4B43 = hook::get_pattern("83 C6 04 4F 75 E7 5F 5E 5B C2 04 00", 0);
    static auto dword_538F62 = hook::get_pattern("8B 4F 7C E8 ? ? ? ? 5F 5E 5D C2 04 00", 8);
    static auto dword_52A984 = hook::get_pattern("8B 4E 28 E8 ? ? ? ? 5E 59 C2 08 00", 0); //night vision 1
    static auto dword_52A98C = hook::get_pattern("8B 4E 28 E8 ? ? ? ? 5E 59 C2 08 00", 8); //night vision 2
    static auto dword_5330C3 = hook::get_pattern("8B 4E 40 E8 ? ? ? ? 5E 59 C2 08 00", 8); //dialog faces

    pattern = hook::pattern("C7 44 24 14 00 00 00 00 DA 4C 24 10 E8 ? ? ? ? 89 44 24 7C 8B 46 30");
    struct HUDHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 0x14) = 0;

            auto ret1 = *(void**)(regs.esp + 0xC8);
            auto ret2 = *(void**)(regs.esp + 0xF4);
            auto ret3 = *(void**)(regs.esp + 0x100);

            if (ret2 == dword_7E2446 && ret3 != dword_63AFDC && ret3 != dword_65E46E && ret3 != dword_65E540 && ret3 != dword_532FF8 && ret3 != dword_5369D3 &&
                ret3 != dword_52E81A && ret3 != dword_5D4B43 && ret3 != dword_538F62 && ret3 != dword_52A984 && ret3 != dword_52A98C)
            {
                *(int32_t*)(regs.esp + 0x70) = (int32_t)(((((Screen.fWidth * (float)(*(int32_t*)(regs.esp + 0x70))) / Screen.fHudScale) + Screen.fHudOffsetReal) / Screen.fWidth) + Screen.fHudOffsetReal);
                *(int32_t*)(regs.esp + 0x78) = (int32_t)((float)(*(int32_t*)(regs.esp + 0x78)) / Screen.fHudScale);

                if (ret3 == dword_5330C3) //dialog faces
                {
                    *(int32_t*)(regs.esp + 0x70) = 20;
                }
            }
        }
    }; injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(8)); //0x

    pattern = hook::pattern("D8 0D ? ? ? ? 8B D9 8B 43 08 56 D9 F2");
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
    }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6)); //0x5E27B8

    //Window style fix
    pattern = hook::pattern("FF 15 ? ? ? ? 8B F8 8B 46 24 85 C0");
    injector::WriteMemory(*pattern.get_first<uint32_t*>(2), CreateWindowExAHook, true); //0x8AB184
    pattern = hook::pattern("FF 15 ? ? ? ? A1 ? ? ? ? 8A 48 24 84 C9");
    injector::WriteMemory(*pattern.get_first<uint32_t*>(2), AdjustWindowRectHook, true); //0x8AB188

    //FMV
    if (bFixFMV)
    {
        auto GetMPEGDimensions = [](fs::path path) -> std::tuple<int32_t, int32_t>
        {
            std::ifstream f(path, std::ifstream::binary);
            if (f)
            {
                uint8_t buffer[1024];
                while (f.read((char*)&(buffer[0]), sizeof(buffer) / sizeof(uint8_t)))
                {
                    auto bytes_read = f.gcount();
                    if (bytes_read > 0)
                    {
                        auto values_read = ((unsigned int)bytes_read) / sizeof(uint32_t);
                        for (unsigned int index = 0U; index < values_read; ++index)
                        {
                            if (buffer[index] == 0x00 && buffer[index + 1] == 0x00 && buffer[index + 2] == 0x01 && buffer[index + 3] == 0xB3)
                            {
                                index += 4;
                                auto w = (buffer[index + 0] * 256 + buffer[index + 1]) >> 4;
                                auto h = (buffer[index + 1] * 256 + buffer[index + 2]) & 0x0FFF;
                                f.close();
                                return std::make_tuple(w, h);
                            }
                        }
                    }
                }
            }
            f.close();
            return std::make_tuple(0, 0);
        };

        auto ConvertMPEG = [](fs::path path)
        {
            auto ffmpeg = GetThisModulePath<std::wstring>() + L"ffmpeg.exe";
            std::wstring params;
            params += L"-y -i \"" + path.wstring() + L"\" ";
            params += L"-filter:v \"pad=ih*" + std::to_wstring(Screen.nWidth) + L"/" + std::to_wstring(Screen.nHeight) + L":ih:(ow-iw)/2:(oh-ih)/2\" ";
            params += L"-c:a copy -f mpeg -q:v 0 ";
            params += L"\"" + path.replace_extension(".wsmpg").wstring() + L"\"";

            SHELLEXECUTEINFOW ShExecInfo = { 0 };
            ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            ShExecInfo.hwnd = NULL;
            ShExecInfo.lpVerb = NULL;
            ShExecInfo.lpFile = ffmpeg.c_str();
            ShExecInfo.lpParameters = params.c_str();
            ShExecInfo.lpDirectory = NULL;
            ShExecInfo.nShow = SW_HIDE;
            ShExecInfo.hInstApp = NULL;
            ShellExecuteExW(&ShExecInfo);
            //WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
        };

        auto path = GetExeModulePath<std::wstring>() + L"data\\video\\";
        for (auto& p : fs::directory_iterator(path))
        {
            if (p.path().extension() == L".mpg")
            {
                auto wsmpg = p.path();
                wsmpg.replace_extension(L".wsmpg");
                if (fs::exists(wsmpg))
                {
                    auto[w, h] = GetMPEGDimensions(wsmpg);
                    int32_t f1 = (int32_t)((roundf(Screen.fAspectRatio * 100) / 100) * 10);
                    int32_t f2 = (int32_t)((roundf(((float)w / (float)h) * 100) / 100) * 10);
                    if (f1 != f2)
                    {
                        ConvertMPEG(p.path());
                    }
                }
                else
                {
                    ConvertMPEG(p.path());
                }
            }
        }

        static constexpr char wsmpg[] = ".wsmpg";
        pattern = hook::pattern("68 ? ? ? ? 52 50 51 8D 54 24 20 68 ? ? ? ? 52 E8 ? ? ? ? 83 C4 18");
        injector::WriteMemory(pattern.get_first(1), &wsmpg, true);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("8D 4C 24 18 50 51 6A 04 8B CE"));

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
    static auto ps2 = PCSX2({ 0x37C182D7, 0x37C182D5 }, NULL, [](PCSX2& ps2)
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
            Screen.fRadarBlipSize = 0.08281249553f / (1.0f / Screen.fHudScale);
            Screen.fRadarBlipPosition = (((Screen.fWidth * 0.865f) / (1.0f / Screen.fHudScale)) + Screen.fHudOffsetReal) / Screen.fWidth;
            Screen.fPointSize = round(2.0f / Screen.fHudScale);
            uint16_t hf = HIWORD(*(uint32_t*)&Screen.fPointSize);
            Screen.PointSizeInstr = (uint32_t)0x3C02 << 16 | (uint32_t)hf;
        });

    while (!ps2.isCRCValid())
        std::this_thread::yield();

    ps2.vecPatches.push_back(PCSX2Memory(L"gametitle=Knight Rider: The Game 2 (PAL-M6)(SLES-52836)"));
    ps2.vecPatches.push_back(PCSX2Memory(L"comment=Widescreen Fix by ThirteenAG https://thirteenag.github.io/wfp#kr2"));
    ps2.vecPatches.push_back(PCSX2Memory(L""));
    ps2.vecPatches.push_back(PCSX2Memory(L"// Current Resolution: " + std::to_wstring(Screen.nWidth) + L"x" + std::to_wstring(Screen.nHeight) + L", Aspect Ratio: " + std::to_wstring(Screen.fAspectRatio)));
    ps2.vecPatches.push_back(PCSX2Memory(L""));

    auto ps2pattern = hook::range_pattern(ps2.EEMainMemoryStart, ps2.EEMainMemoryEnd, "AA 3F ? 3C");
    while (ps2pattern.clear().size() != 11) { std::this_thread::yield(); }
    for (size_t i = 0; i < ps2pattern.size(); i++)
    {
        ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2pattern.get(i).get<void>(0), WORD_T, LUI_ORI, &Screen.fAspectRatio,
            std::wstring(L"// Aspect Ratio: ") + std::to_wstring(Screen.fAspectRatio), 0x3FAAAAAB));
    }

    ps2pattern = hook::range_pattern(ps2.EEMainMemoryStart, ps2.EEMainMemoryEnd, "0E 3C 02 3C D0 FF BD 27 36 FA 42 34");
    while (ps2pattern.clear().size() != 2) { std::this_thread::yield(); }
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2pattern.get(0).get<void>(0), WORD_T, LUI_ORI, &Screen.fFOVFactor, std::wstring(L"// FOV: ") + std::to_wstring(Screen.fFOVFactor), 0x3C0EFA36));

    ps2.WritePnach(); //writing pnach with addresses so far

    ps2pattern = hook::range_pattern(ps2.EEMainMemoryStart, ps2.EEMainMemoryEnd, "00 40 02 3C E0 00 40 C6 57 00 05 3C"); //lui $v0, 0x4000
    while (ps2pattern.clear().size() != 1) { std::this_thread::yield(); }
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2pattern.get(0).get<void>(0), WORD_T, NONE, &Screen.PointSizeInstr, std::wstring(L"// Point Size Instr: ") + int_to_hex(Screen.PointSizeInstr)));

    //HUD
    ps2pattern = hook::range_pattern(ps2.EEMainMemoryStart, ps2.EEMainMemoryEnd, "00 00 80 3F 2D BD 3B 33 E2 CD 0C A8 48 C1 65 BD");
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE_DYN, PCSX2MemoryDyn(ps2pattern, 1, 0, 0), WORD_T, NONE, &Screen.fHudScale, std::wstring(L"// Hud Scale: ") + std::to_wstring(Screen.fHudScale)));

    ps2pattern = hook::range_pattern(ps2.EEMainMemoryStart, ps2.EEMainMemoryEnd, "7D AA 8A 3D ? ? ? ? 8C B9 5B 3F");
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE_DYN, PCSX2MemoryDyn(ps2pattern, 1, 0, 4), WORD_T, NONE, &Screen.fRadarBlipPosition, std::wstring(L"// Radar Blip Position: ") + std::to_wstring(Screen.fRadarBlipPosition)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE_DYN, PCSX2MemoryDyn(ps2pattern, 1, 0, -4), WORD_T, NONE, &Screen.fRadarBlipSize, std::wstring(L"// Radar Blip Size: ") + std::to_wstring(Screen.fRadarBlipSize)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE_DYN, PCSX2MemoryDyn(ps2pattern, 1, 0, 44), WORD_T, NONE, &Screen.fAspectRatio, std::wstring(L"// Hud Aspect Ratio: ") + std::to_wstring(Screen.fAspectRatio)));

    ps2.WriteMemoryLoop();
}