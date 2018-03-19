#include "stdafx.h"
#include "ico.h"
#include <d3d9.h>
#include <D3dx9.h>
#pragma comment(lib, "D3dx9.lib")

//#define _LOG
#ifdef _LOG
#include <fstream>
ofstream logfile;
uint32_t logit;
#endif // _LOG

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fDynamicScreenFieldOfViewScale;
    float fHudOffset;
    float fAspectRatio;
    float fHudScaleX;
    float fTextScaleX;
    int32_t FilmstripScaleX;
    int32_t FilmstripOffset;
    uint32_t pFilmstripTex;
} Screen;

std::string szLoadscPath;
HICON ico;
HICON WINAPI LoadIconProxy(HINSTANCE hInstance, LPCWSTR lpIconName)
{
    return ico;
}

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

    //ScopeLens
    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8B 80 ? ? ? ? 8B 08 6A 14 8D 54 24 70 52 6A 02 6A 05");
    struct ScopeLens_Hook
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
    }; injector::MakeInline<ScopeLens_Hook>(pattern.get_first(0), pattern.get_first(6));

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
                            if (D3DXCreateTextureFromFile(ppDevice, szLoadscPath.c_str(), &pTexLoadscreenCustom) == D3D_OK)
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

            if ((w == 1275.0f && h == 637.0f) || (w == 32.0f && h == 32.0f)) // fullscreen loadscreen and spinner
            {
                if (!(w == 32.0f && h == 32.0f))
                {
                    *(float*)&dword_109E09F4[regs.eax / 4] *= Screen.fHudScaleX;
                    *(float*)&dword_109E09FC[regs.eax / 4] *= Screen.fHudScaleX;
                }
            }
            else
            {
                if (w == 800.0f)
                {
                    *(float*)&dword_109E09F0[regs.eax / 4] *= Screen.fHudScaleX;
                    *(float*)&dword_109E09F8[regs.eax / 4] *= Screen.fHudScaleX;
                }
                else
                {
                    *(float*)&dword_109E09F0[regs.eax / 4] /= Screen.fHudScaleX;
                    *(float*)&dword_109E09F8[regs.eax / 4] /= Screen.fHudScaleX;
                }
            }
        }
    }; injector::MakeInline<ImageUnrealDrawHook>(pattern.get_first(0), pattern.get_first(104));

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
#undef SCREEN_FOV_HORIZONTAL
#undef SCREEN_FOV_VERTICAL
#define SCREEN_FOV_HORIZONTAL 75.0f
#define SCREEN_FOV_VERTICAL (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))
    Screen.fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);

    pattern = hook::module_pattern(GetModuleHandle("Engine"), "8B 91 28 06 00 00 52 8B"); //?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z  10530BD7
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto f = *(float*)(regs.ecx + 0x628) * Screen.fDynamicScreenFieldOfViewScale;
            *(float*)&regs.edx = f > 170.0f ? 170.0f : f;
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

    injector::WriteMemory(*pattern.count_hint(2).get(0).get<void*>(2), LoadIconProxy, true);
    injector::WriteMemory(*pattern.count_hint(2).get(1).get<void*>(2), LoadIconProxy, true);

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

    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebp + 0x61D8) = regs.eax;

            CIniReader iniReader("");
            Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
            Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
            bool bForceLL = iniReader.ReadInteger("MAIN", "ForceLL", 1) != 0;
            szLoadscPath = iniReader.GetIniPath();
            szLoadscPath = szLoadscPath.substr(0, szLoadscPath.find_last_of('.')) + ".png";

            if (bForceLL)
            {
                auto pattern = hook::pattern("74 ? 68 ? ? ? ? 53 FF D7");
                injector::MakeNOP(pattern.get_first(), 2, true);
            }

            if (!Screen.Width || !Screen.Height)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHudScaleX = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fTextScaleX = 600.0f * Screen.fAspectRatio;
            Screen.fHudOffset = ((4.0f / 3.0f) / Screen.fAspectRatio);
            Screen.FilmstripScaleX = static_cast<int32_t>(Screen.fWidth / (1280.0f / (368.0 * ((4.0 / 3.0) / (Screen.fAspectRatio)))));
            Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * (4.0f / 3.0f)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));

            if (Screen.Width && Screen.Height)
            {
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
            }

            InitEngine(NULL);
            InitD3DDrv(NULL);
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));

    //icon fix
    ico = CreateIconFromBMP(icoData);
    InitWindow(NULL);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
#ifdef _LOG
        logfile.open("SC4.WidescreenFix.log");
#endif // _LOG
        Init(NULL);
    }
    return TRUE;
}