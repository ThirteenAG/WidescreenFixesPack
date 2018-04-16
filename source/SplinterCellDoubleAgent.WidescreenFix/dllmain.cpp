#include "stdafx.h"
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

void InitLL()
{
    auto pattern = hook::pattern("74 ? 68 ? ? ? ? 53 FF D7");
    injector::MakeNOP(pattern.get_first(), 2, true);
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bForceLL = iniReader.ReadInteger("MAIN", "ForceLL", 1) != 0;
    Screen.szLoadscPath = iniReader.GetIniPath();
    Screen.szLoadscPath = Screen.szLoadscPath.substr(0, Screen.szLoadscPath.find_last_of('.')) + ".png";

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    char UserIni[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle(NULL), UserIni, (sizeof(UserIni)));
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

    if (bForceLL)
        CallbackHandler::RegisterCallback(InitLL, hook::pattern("74 ? 68 ? ? ? ? 53 FF D7").count_hint(1).empty(), 0x1100);

    //for test only (steam version)
    //CallbackHandler::RegisterCallback([]()
    //{
    //	auto pattern = hook::pattern("89 85 D8 61 00 00");
    //	struct StartupHook
    //	{
    //		void operator()(injector::reg_pack& regs)
    //		{
    //			*(uint32_t*)(regs.ebp + 0x61D8) = regs.eax;
    //			MessageBox(0, 0, L"test", 0);
    //		}
    //	}; injector::MakeInline<StartupHook>(pattern.get_first(0), pattern.get_first(6));
    //}, hook::pattern("89 85 D8 61 00 00").count_hint(1).empty(), 0x1100);
}

void InitD3DDrv()
{
    //Minimap aka FilmStrip
    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 4B 04 03 C9 85 C9 89 4C 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x38C
    struct RenderFilmstrip_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.FilmstripScaleX + Screen.FilmstripScaleX;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook>(pattern.get_first(0));

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "03 D2 85 D2 89 54 24 10 DB 44 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x350
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
    static auto pPresentParams = *hook::module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? F3 AB 8B 0D ? ? ? ? 39 01 8B 4D").get_first<D3DPRESENT_PARAMETERS*>(1);
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 01 00 00 00 64 89 0D 00 00 00 00 5B 8B E5 5D C2 10 00");
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
                SetForegroundWindow(pPresentParams->hDeviceWindow);
                SetCursor(NULL);
            }
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0));

    //ScopeLens
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 80 ? ? ? ? 8B 08 6A 14 8D 54 24 70 52 6A 02 6A 05");
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
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A 04 8B CE F3 0F 11 44 24"); //
    struct RenderFilmstrip_Hook3
    {
        void operator()(injector::reg_pack& regs)
        {
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
            static float _xmm0 = 0.0f;
            _asm movss   dword ptr[_xmm0], xmm0
            *(float*)(regs.esp + 0x18) = _xmm0;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook3>(pattern.get_first(4), pattern.get_first(10));

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 46 2C 3B C7 74 1D 39 7E 4C 50"); //
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

    //loadscreen
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "89 BE ? ? ? ? E8 ? ? ? ? 8B 86 ? ? ? ? 8B 08 57 6A 00 50 FF 91 ? ? ? ? 6A 04 8B CB E8 ? ? ? ? 8B 6C 24 30 F6 45 54 01 74 48 83 BE"); //
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
                            if (D3DXCreateTextureFromFileA(ppDevice, Screen.szLoadscPath.c_str(), &pTexLoadscreenCustom) == D3D_OK)
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

    //Enhanced night vision NaN workaround
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F3 A5 8B 90 30 02 00 00");
    injector::MakeNOP(pattern.get_first(0), 2, true);
}

void InitEngine()
{
    static bool bIsOPSAT = false;
    static bool bIsVideoPlaying = false;
    static auto GIsWideScreen = *hook::module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? F3 0F 59 D9 F3 0F 10 0D ? ? ? ? F3 0F 5C D8 F3 0F 58 DC").get_first<uint8_t*>(2);
    static auto GIsSameFrontBufferOnNormalTV = *hook::module_pattern(GetModuleHandle(L"Engine"), "8B 15 ? ? ? ? 83 3A 00 74 58 F3 0F 10 88").get_first<uint8_t*>(2);
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "83 39 00 75 63 8B 15 ? ? ? ? 83 3A 00"); //
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
            else
            {
                if (bIsOPSAT && bIsVideoPlaying)
                {
                    *(float*)&dword_109E09F0[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09F4[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09F8[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09FC[regs.eax / 4] *= (4.0f / 3.0f);
                }
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

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "D9 05 ? ? ? ? 83 C4 08 D8 C9 D9 5C 24 24");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "D9 05 ? ? ? ? D8 4C 24 10 D9 5C 24 28");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 10 0D ? ? ? ? F3 0F 11 44 24 ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ? F3 0F 11 4C 24 ? F3 0F 11 44 24");
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 10 0D ? ? ? ? F3 0F 10 15 ? ? ? ? 2B D1 F3 0F 2A C2");
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C D8 F3 0F 58 DC"); //0x1030503C
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

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5E CA F3 0F 11 4C 24 14 83 C4 04"); //0x1030678D
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

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? 83 39 00 89 54 24 20 75 3D 8B 15 ? ? ? ? 83 3A 00");
    struct TextHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            auto f3 = 0.0f;
            auto f4 = *(float*)(regs.esp + 0x24);
            auto f5 = 0.0f;
            auto f6 = 0.0f;
            _asm { movss dword ptr ds : f3, xmm3}
            _asm { movss dword ptr ds : f5, xmm5}
            _asm { movss dword ptr ds : f6, xmm6}

            if (!*GIsWideScreen && *GIsSameFrontBufferOnNormalTV)
            {
                *(float *)(regs.ebp + 0x00) = f3 * 0.75f;
                *(float *)(regs.ebp + 0x04) = f4 * 0.75f;
                *(float *)(regs.ebp + 0x08) = f5 * 0.75f;
                *(float *)(regs.ebp + 0x0C) = f6 * 0.75f;
            }
            else
            {
                if (bIsOPSAT && bIsVideoPlaying)
                {
                    *(float *)(regs.ebp + 0x00) = f3 * (4.0f / 3.0f);
                    *(float *)(regs.ebp + 0x04) = f4 * (4.0f / 3.0f);
                    *(float *)(regs.ebp + 0x08) = f5 * (4.0f / 3.0f);
                    *(float *)(regs.ebp + 0x0C) = f6 * (4.0f / 3.0f);
                }
            }
        }
    }; injector::MakeInline<TextHook2>(pattern.get_first(0), pattern.get_first(76));

    //OPSAT and computer videos (scale text and hud like x360 version)
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 86 ? ? ? ? 83 C4 10 85 C0 74 49 83 78 64 00"); //0x10515086
    struct UCanvasOpenVideoHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static auto getVideoPath = [](const std::string_view& str, char beg, char end) -> std::string
            {
                std::size_t begPos;
                if ((begPos = str.find(beg)) != std::string_view::npos)
                {
                    std::size_t endPos;
                    if ((endPos = str.find(end, begPos)) != std::string_view::npos && endPos != begPos + 1)
                        return std::string(str).substr(begPos + 1, endPos - begPos - 1);
                }

                return std::string();
            };
            bIsVideoPlaying = true;
            regs.eax = *(uint32_t*)(regs.esi + 0x80);
            auto str = getVideoPath(std::string_view(*(char**)(regs.esp + 0x0C)), '\\', '.');
            if (iequals(str, "computer_hq_1") || iequals(str, "computer_hq_2") || iequals(str, "computer_hq_3") ||
                iequals(str, "computer_mission_1") || iequals(str, "computer_mission_2") || starts_with(str.c_str(), "opsat", false))
                bIsOPSAT = true;
            else
                bIsOPSAT = false;
        }
    }; injector::MakeInline<UCanvasOpenVideoHook>(pattern.get_first(0), pattern.get_first(6));

    //
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 86 ? ? ? ? 85 C0 74 12 83 78 64 00 74 0C"); //0x10515110
    struct UCanvasCloseVideoHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi + 0x80);
            bIsVideoPlaying = false;
        }
    }; injector::MakeInline<UCanvasCloseVideoHook>(pattern.get_first(0), pattern.get_first(6));

    //FOV
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 91 28 06 00 00 52 8B"); //?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z  10530BD7
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.edx = AdjustFOV(*(float*)(regs.ecx + 0x628), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(0), pattern.get_first(6));
}

void InitWindow()
{
    //icon fix
    auto RegisterClassExWProxy = [](WNDCLASSEXW* lpwcx) -> ATOM {
        lpwcx->hIcon = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
        lpwcx->hIconSm = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
        return RegisterClassExW(lpwcx);
    };

    auto RegisterClassExAProxy = [](WNDCLASSEXA* lpwcx) -> ATOM {
        lpwcx->hIcon = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
        lpwcx->hIconSm = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
        return RegisterClassExA(lpwcx);
    };

    auto pattern = hook::module_pattern(GetModuleHandle(L"Window"), "FF 15 ? ? ? ? 66 85 C0 0F 85 ? ? ? ? 68 F5 02 00 00");
    injector::WriteMemory(*pattern.get_first<void*>(2), static_cast<ATOM(WINAPI*)(WNDCLASSEXW*)>(RegisterClassExWProxy), true);
    pattern = hook::module_pattern(GetModuleHandle(L"Window"), "FF 15 ? ? ? ? 66 85 C0 75 18 68 03 03 00 00");
    injector::WriteMemory(*pattern.get_first<void*>(2), static_cast<ATOM(WINAPI*)(WNDCLASSEXA*)>(RegisterClassExAProxy), true);
}

void InitEchelonMenus()
{
    static std::vector<std::string> list;
    GetResolutionsList(list);
    std::swap(list[list.size() - 5], list[list.size() - 6]);
    list[list.size() - 5] = format("%dx%d", Screen.Width, Screen.Height);

    auto pattern = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "C6 45 FC 10 33 F6 83 FE 07");
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
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"EchelonMenus.dll", InitEchelonMenus);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}