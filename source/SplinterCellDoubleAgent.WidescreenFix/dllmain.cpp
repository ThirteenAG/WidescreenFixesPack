#include "stdafx.h"
#include "ico.h"
#include <d3d9.h>
#include <D3dx9.h>
#pragma comment(lib, "D3dx9.lib")

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fHudOffset;
    float fAspectRatio;
    float fHudScaleX;
    float fHudScaleX2;
    float fTextScaleX;
    int32_t FilmstripScaleX;
    int32_t FilmstripOffset;
    uint32_t pFilmstripTex;
    std::string szLoadscPath;
} Screen;

DWORD WINAPI InitD3DDrv(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8B 4B 04 03 C9 85 C9 89 4C 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x38C

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitD3DDrv, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("D3DDrv")).count_hint(1).empty()) { Sleep(0); };

    //Minimap aka FilmStrip
    struct RenderFilmstrip_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.FilmstripScaleX + Screen.FilmstripScaleX;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook>(pattern.get_first(0));

    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "03 D2 85 D2 89 54 24 10 DB 44 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x350
    struct RenderFilmstrip_Hook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = Screen.FilmstripOffset;
            *(int32_t*)(regs.esp + 0x10) = regs.edx;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook2>(pattern.get_first(0), pattern.get_first(6));
    injector::WriteMemory<uint16_t>(pattern.get_first(6), 0xD285, true);     //test    edx, edx

    //SetRes
    static auto pPresentParams = *hook::module_pattern(GetModuleHandle("D3DDrv"), "BF ? ? ? ? F3 AB 8B 0D ? ? ? ? 39 01 8B 4D").get_first<D3DPRESENT_PARAMETERS*>(1);
    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "B8 01 00 00 00 64 89 0D 00 00 00 00 5B 8B E5 5D C2 10 00");
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 1;

            Screen.Width = pPresentParams->BackBufferWidth;
            Screen.Height = pPresentParams->BackBufferHeight;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHudScaleX = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fHudScaleX2 = 2.0f / (600.0f * Screen.fAspectRatio);
            Screen.fTextScaleX = 600.0f * Screen.fAspectRatio;
            Screen.fHudOffset = ((4.0f / 3.0f) / Screen.fAspectRatio);
            Screen.FilmstripScaleX = static_cast<int32_t>(Screen.fWidth / (1280.0f / (368.0 * ((4.0 / 3.0) / (Screen.fAspectRatio)))));
            Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * (4.0f / 3.0f)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));

            CIniReader iniReader("");
            auto[DesktopResW, DesktopResH] = GetDesktopRes();
            if (Screen.Width != DesktopResW || Screen.Height != DesktopResH)
            {
                iniReader.WriteInteger("MAIN", "ResX", Screen.Width);
                iniReader.WriteInteger("MAIN", "ResY", Screen.Height);
            }
            else
            {
                iniReader.WriteInteger("MAIN", "ResX", 0);
                iniReader.WriteInteger("MAIN", "ResY", 0);
            }

            if (pPresentParams->Windowed)
            {
                tagRECT rect;
                rect.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
                rect.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.fHeight / 2.0f));
                rect.right = (LONG)Screen.Width;
                rect.bottom = (LONG)Screen.Height;
                SetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE, GetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(pPresentParams->hDeviceWindow, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0));

    //ScopeLens
    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8B 80 ? ? ? ? 8B 08 6A 14 8D 54 24 70 52 6A 02 6A 05");
    struct ScopeLensHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.eax + 0x5F30);

            if (Screen.fAspectRatio > (4.0f / 3.0f))
            {
                auto pVertexStreamZeroData = regs.esp + 0x6C;
                *(float*)(pVertexStreamZeroData + 0x00) /= (Screen.fAspectRatio / (16.0f / 9.0f)) / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
                *(float*)(pVertexStreamZeroData + 0x14) /= (Screen.fAspectRatio / (16.0f / 9.0f)) / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
                *(float*)(pVertexStreamZeroData + 0x28) /= (Screen.fAspectRatio / (16.0f / 9.0f)) / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
                *(float*)(pVertexStreamZeroData + 0x3C) /= (Screen.fAspectRatio / (16.0f / 9.0f)) / (((4.0f / 3.0f)) / (Screen.fAspectRatio));

                *(float*)(pVertexStreamZeroData + 0x04) /= (4.0f / 3.0f);
                *(float*)(pVertexStreamZeroData + 0x18) /= (4.0f / 3.0f);
                *(float*)(pVertexStreamZeroData + 0x2C) /= (4.0f / 3.0f);
                *(float*)(pVertexStreamZeroData + 0x40) /= (4.0f / 3.0f);
            }
        }
    }; injector::MakeInline<ScopeLensHook1>(pattern.get_first(0), pattern.get_first(6));

    //crashfix
    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8D B5 64 98 00 00 6A 04 8B CE F3 0F 11 44 24 18"); //
    struct RenderFilmstrip_Hook3
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = regs.ebp + 0x9864;

            if (regs.edi)
            {
                Screen.pFilmstripTex = regs.edi;
                auto pTex = (IDirect3DTexture9*)regs.edi;
                auto type = pTex->GetType();

                if (type == 0) // check if ptr valid so it doesn't crash
                {
                    regs.edi = 0;
                    *(uint32_t*)(regs.ebx + 0x3C) = regs.edi;
                }
            }
        }
    }; injector::MakeInline<RenderFilmstrip_Hook3>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8B 46 2C 3B C7 74 1D 39 7E 4C 50"); //
    static auto GMalloc = *pattern.get_first<uint32_t>(22);
    struct FlushHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi + 0x2C);
            if (regs.eax && regs.eax != Screen.pFilmstripTex)
            {
                regs.eax = *(uint32_t*)(regs.esi + 0x2C);
                if (*(uint32_t*)(regs.esi + 0x4C))
                    (*(void(__stdcall**)(int))(*(uint32_t*)GMalloc + 8))(regs.eax);
                else
                    (*(void(__stdcall**)(int))(*(uint32_t*)regs.eax + 8))(regs.eax);
                *(uint32_t*)(regs.esi + 0x2C) = 0;
            }
        }
    }; injector::MakeInline<FlushHook>(pattern.get_first(0), pattern.get_first(36));

    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8B 95 ? ? ? ? 8B 85 ? ? ? ? 8B 08 52 50 FF 91 ? ? ? ? 8B 8D"); //
    struct RenderFilmstripHook4 //vertex shader for alt tab
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = NULL;
        }
    }; injector::MakeInline<RenderFilmstripHook4>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8B 7B 64 85 FF 8B CE 6A 04 74 1F 89 BD"); //alpha, settexture, maybe not needed
    injector::WriteMemory(pattern.get_first(0), 0x85909090, true);

    //pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8D 9E 64 98 00 00 6A 04 8B CB 89 BE B0 6E 00 00 E8 ? ? ? ? 8B 86 30 5F 00 00 8B 08 57 6A 00 50 FF 91 04 01 00 00 6A 04 8B CB E8 ? ? ? ? 8B 6C 24 30 F6 45 54 01 74 48 83 BE 64 60 00 00 00");
    //struct test
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        regs.ebx = regs.esi + 0x9864;
    //    }
    //}; injector::MakeInline<test>(pattern.get_first(0), pattern.get_first(6));

    //loadscreen
    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "89 BE ? ? ? ? E8 ? ? ? ? 8B 86 ? ? ? ? 8B 08 57 6A 00 50 FF 91 ? ? ? ? 6A 04 8B CB E8 ? ? ? ? 8B 6C 24 30 F6 45 54 01 74 48 83 BE"); //
    struct LoadscHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (regs.edi)
            {
                auto pTex = (IDirect3DTexture9*)regs.edi;
                if (pTex->GetType() == D3DRTYPE_TEXTURE)
                {
                    D3DSURFACE_DESC pDesc;
                    pTex->GetLevelDesc(0, &pDesc);
                    if (pDesc.Width == 2048 && pDesc.Height == 1024 && pDesc.Format == D3DFMT_DXT5 && pDesc.Usage == 0)
                    {
                        static LPDIRECT3DTEXTURE9 pTexLoadscreenCustom = nullptr;
                        if (!pTexLoadscreenCustom)
                        {
                            IDirect3DDevice9* ppDevice = nullptr;
                            pTex->GetDevice(&ppDevice);
                            if (D3DXCreateTextureFromFile(ppDevice, Screen.szLoadscPath.c_str(), &pTexLoadscreenCustom) == D3D_OK)
                                regs.edi = (uint32_t)pTexLoadscreenCustom;
                        }
                        else
                            regs.edi = (uint32_t)pTexLoadscreenCustom;
                    }
                }
            }
            *(uint32_t*)(regs.esi + 0x6EB0) = regs.edi;
        }
    }; injector::MakeInline<LoadscHook>(pattern.get_first(0), pattern.get_first(6));

    return 0;
}

DWORD WINAPI InitEngine(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 11 15 ? ? ? ? 0F BF"); //10305753

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitEngine, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("Engine")).count_hint(1).empty()) { Sleep(0); };

    static auto GIsWideScreen = *hook::module_pattern(GetModuleHandle("Engine"), "8B 0D ? ? ? ? F3 0F 59 D9 F3 0F 10 0D ? ? ? ? F3 0F 5C D8 F3 0F 58 DC").get_first<uint8_t*>(2);
    static auto GIsSameFrontBufferOnNormalTV = *hook::module_pattern(GetModuleHandle("Engine"), "8B 15 ? ? ? ? 83 3A 00 74 58 F3 0F 10 88").get_first<uint8_t*>(2);
    pattern = hook::module_pattern(GetModuleHandle("Engine"), "83 39 00 75 63 8B 15 ? ? ? ? 83 3A 00"); //
    static auto dword_109E09F0 = *pattern.get_first<float*>(60);
    static auto dword_109E09F4 = dword_109E09F0 + 1;
    static auto dword_109E09F8 = dword_109E09F0 + 2;
    static auto dword_109E09FC = dword_109E09F0 + 3;
    struct ImageUnrealDrawHook
    {
        void operator()(injector::reg_pack& regs)
        {
            //auto x = *(float*)&dword_109E09F0[regs.eax / 4];
            //auto y = *(float*)&dword_109E09F4[regs.eax / 4];
            auto w = *(float*)&dword_109E09F8[regs.eax / 4];
            auto h = *(float*)&dword_109E09FC[regs.eax / 4];

            if (!*GIsWideScreen && *GIsSameFrontBufferOnNormalTV)
            {
                *(float*)&dword_109E09F0[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09F4[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09F8[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09FC[regs.eax / 4] *= 0.75f;
            }

            if ((w == 1275.0f && h == 637.0f) || (w == 800.0f)) // loadscreen and fullscreen images
            {
                *(float*)&dword_109E09F0[regs.eax / 4] *= Screen.fHudScaleX;
                *(float*)&dword_109E09F8[regs.eax / 4] *= Screen.fHudScaleX;

                if (w == 1275.0f && h == 637.0f)
                {
                    *(float*)&dword_109E09F4[regs.eax / 4] *= Screen.fHudScaleX;
                    *(float*)&dword_109E09FC[regs.eax / 4] *= Screen.fHudScaleX;
                }
            }
        }
    }; injector::MakeInline<ImageUnrealDrawHook>(pattern.get_first(0), pattern.get_first(104));

    pattern = hook::module_pattern(GetModuleHandle("Engine"), "D9 05 ? ? ? ? 83 C4 08 D8 C9 D9 5C 24 24");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle("Engine"), "D9 05 ? ? ? ? D8 4C 24 10 D9 5C 24 28");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 10 0D ? ? ? ? F3 0F 11 44 24 ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ? F3 0F 11 4C 24 ? F3 0F 11 44 24");
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 10 0D ? ? ? ? F3 0F 10 15 ? ? ? ? 2B D1 F3 0F 2A C2");
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);

    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 5C D8 F3 0F 58 DC"); //0x1030503C
    struct HUDPosHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static const float f1_0 = 1.0f;
            _asm
            {
                movss xmm0, Screen.fHudOffset
                subss xmm3, xmm0
                addss xmm3, xmm4
                movss xmm0, f1_0
            }
        }
    }; injector::MakeInline<HUDPosHook>(pattern.get_first(0), pattern.get_first(8));

    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 5E CA F3 0F 11 4C 24 14 83 C4 04"); //0x1030678D
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto regs_ecx = regs.ecx;
            _asm
            {
                cvtsi2ss xmm2, regs_ecx
                movss xmm2, Screen.fTextScaleX
                divss xmm1, xmm2
            }
        }
    }; injector::MakeInline<TextHook>(pattern.get_first(-4), pattern.get_first(4));

    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);

    //FOV
    pattern = hook::module_pattern(GetModuleHandle("Engine"), "8B 91 28 06 00 00 52 8B"); //?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z  10530BD7
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.edx = AdjustFOV(*(float*)(regs.ecx + 0x628), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(0), pattern.get_first(6));

    return 0;
}

DWORD WINAPI InitWindow(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("Window"), "FF 15 ? ? ? ? 8B 4E 04 50 6A 01 68 80 00 00 00 51");

    if (pattern.count_hint(2).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitWindow, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("Window")).count_hint(2).empty()) { Sleep(0); };

    //icon fix
    static auto ico = CreateIconFromBMP(icoData);
    auto LoadIconProxy = [](HINSTANCE hInstance, LPCWSTR lpIconName) -> HICON {
        return ico;
    };

    injector::WriteMemory(*pattern.count_hint(2).get(0).get<void*>(2), static_cast<HICON(WINAPI *)(HINSTANCE, LPCWSTR)>(LoadIconProxy), true);
    injector::WriteMemory(*pattern.count_hint(2).get(1).get<void*>(2), static_cast<HICON(WINAPI *)(HINSTANCE, LPCWSTR)>(LoadIconProxy), true);

    return 0;
}

DWORD WINAPI InitEchelonMenus(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("EchelonMenus"), "C6 45 FC 10 33 F6 83 FE 07");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitEchelonMenus, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("EchelonMenus")).count_hint(1).empty()) { Sleep(0); };

    static std::vector<std::string> list;
    GetResolutionsList(list);
    std::swap(list[list.size() - 5], list[list.size() - 6]);
    list[list.size() - 5] = format("%dx%d", Screen.Width, Screen.Height);

    auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(-850), (uintptr_t)pattern.get_first(0), "C7 45 E8");
    for (size_t i = 0; i < rpattern.size(); ++i)
    {
        injector::WriteMemory(rpattern.get(i).get<uint32_t>(3), &list[list.size() - 1 - i][0], true);
    }

    static const auto wsz0 = 0;
    rpattern = hook::range_pattern((uintptr_t)pattern.get_first(-850), (uintptr_t)pattern.get_first(0), "8D ? E4 68");
    for (size_t i = 0; i < rpattern.size(); ++i)
    {
        injector::WriteMemory(rpattern.get(i).get<uint32_t>(4), &wsz0, true);
    }

    struct ResListHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.ebp - 4) = 16;
            regs.esi = 0;
            list.clear();
        }
    }; injector::MakeInline<ResListHook>(pattern.get_first(0), pattern.get_first(6));

    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("89 85 D8 61 00 00");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    struct StartupHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebp + 0x61D8) = regs.eax;

            CIniReader iniReader("");
            Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
            Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
            bool bForceLL = iniReader.ReadInteger("MAIN", "ForceLL", 1) != 0;
            Screen.szLoadscPath = iniReader.GetIniPath();
            Screen.szLoadscPath = Screen.szLoadscPath.substr(0, Screen.szLoadscPath.find_last_of('.')) + ".png";

            if (bForceLL)
            {
                auto pattern = hook::pattern("74 ? 68 ? ? ? ? 53 FF D7");
                injector::MakeNOP(pattern.get_first(), 2, true);
            }

            if (!Screen.Width || !Screen.Height)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

            char UserIni[MAX_PATH];
            GetModuleFileName(GetModuleHandle(NULL), UserIni, (sizeof(UserIni)));
            *strrchr(UserIni, '\\') = '\0';
            strcat(UserIni, "\\SplinterCell4.ini");

            CIniReader iniWriter(UserIni);
            char ResX[20];
            char ResY[20];
            _snprintf(ResX, 20, "%d", Screen.Width);
            _snprintf(ResY, 20, "%d", Screen.Height);
            iniWriter.WriteString("WinDrv.WindowsClient", "WindowedViewportX", ResX);
            iniWriter.WriteString("WinDrv.WindowsClient", "WindowedViewportY", ResY);
            iniWriter.WriteString("WinDrv.WindowsClient", "FullscreenViewportX", ResX);
            iniWriter.WriteString("WinDrv.WindowsClient", "FullscreenViewportY", ResY);

            InitEngine(NULL);
            InitD3DDrv(NULL);
            InitEchelonMenus(NULL);
        }
    }; injector::MakeInline<StartupHook>(pattern.get_first(0), pattern.get_first(6));

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
        InitWindow(NULL);
    }
    return TRUE;
}