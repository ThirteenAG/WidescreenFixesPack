#include "stdafx.h"
#include "GTA\CFileMgr.h"
#include <d3d9.h>
#include "dxsdk\d3dvtbl.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    int32_t nWidth43;
    float fWidth43;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScaleX;
    float fHudPosX;
    bool bDrawBorders;
    float fHudOffsetReal;
} Screen;

union HudPos
{
    uint32_t dwPos;
    float fPos;
    HudPos(uint32_t pos) : dwPos(pos) {}
    HudPos(float pos) : fPos(pos) {}
};

class CDatEntry
{
public:
    int32_t dwPosX;
    int32_t dwPosY;
    float fOffsetX;
    float fOffsetY;

    CDatEntry(uint32_t posx, uint32_t posy, float fOffsetX, float fOffsetY)
        : dwPosX(posx), dwPosY(posy), fOffsetX(fOffsetX), fOffsetY(fOffsetY)
    {}
};

void LoadDatFile(std::string_view str, std::function<void(std::string_view line)>&& cb)
{
    if (FILE* hFile = CFileMgr::OpenFile(str.data(), "r"))
    {
        while (const char* pLine = CFileMgr::LoadLine(hFile))
        {
            if (pLine[0] && pLine[0] != '#')
            {
                cb(pLine);
            }
        }
        CFileMgr::CloseFile(hFile);
    }
}

std::reference_wrapper<int32_t> nGameState = std::ref(Screen.nWidth);

typedef HRESULT(STDMETHODCALLTYPE* CreateDevice_t)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
CreateDevice_t RealD3D9CreateDevice = NULL;
typedef HRESULT(STDMETHODCALLTYPE* EndScene_t)(LPDIRECT3DDEVICE9);
EndScene_t RealEndScene = NULL;

void DrawRect(LPDIRECT3DDEVICE9 pDevice, int32_t x, int32_t y, int32_t w, int32_t h, D3DCOLOR color = D3DCOLOR_XRGB(0, 0, 0))
{
    D3DRECT BarRect = { x, y, x + w, y + h };
    pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 0, 0);
}

HRESULT WINAPI EndScene(LPDIRECT3DDEVICE9 pDevice)
{
    if (nGameState == 3)
    {
        DrawRect(pDevice, 0, 0, static_cast<int32_t>(Screen.fHudOffsetReal), Screen.nHeight);
        DrawRect(pDevice, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal), 0, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal + Screen.fHudOffsetReal), Screen.nHeight);
    }

    return RealEndScene(pDevice);
}

HRESULT __stdcall CreateDevice(IDirect3D9* d3ddev, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
    HRESULT retval = RealD3D9CreateDevice(d3ddev, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)*ppReturnedDeviceInterface));
    RealEndScene = (EndScene_t)pVTable[IDirect3DDevice9VTBL::EndScene];
    injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::EndScene], &EndScene, true);
    return retval;
}


void*(*CreateResourceFile)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int);
void(__fastcall *ResourceFileBeginLoading)(int a1, void* rsc, int a2);
void LoadResourceFile(const char* ResourceFileName, int32_t ResourceFileType, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
{
    auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
    _asm
    {
        mov     edx, r
        mov     ecx, nUnk5
        mov     eax, nUnk4
        call    ResourceFileBeginLoading
    }
};

uint32_t* dword_73645C;
uint8_t* byte_735768;
void* off_71AA4C;

void __declspec(naked) CmpShouldClearSurfaceDuring3DRender()
{
    _asm
    {
        mov eax, dword_73645C
        cmp dword ptr [eax], 1
        jne resume

        // byte_735768 is set if the magazine view screen is open.
        mov eax, byte_735768
        cmp byte ptr [eax], 0

        resume:
        ret
    }
}


void __declspec(naked) MaybeClearSurfaceDuringBackgroundRender()
{
    _asm
    {
        call CmpShouldClearSurfaceDuring3DRender
        jz resume

        // In case 3D rendering won't clear the render surface, it must
        // be done during the 2D background rendering.
        mov dword ptr [esp+4], 1

        resume:
        mov esi, off_71AA4C
        ret
    }
}

void Init()
{
    CIniReader iniReader("");
    Screen.nWidth = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.nHeight = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bScaling = iniReader.ReadInteger("MAIN", "Scaling", 1);
    bool bHUDWidescreenMode = iniReader.ReadInteger("MAIN", "HUDWidescreenMode", 1) != 0;
    int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
    static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    bool bD3DHookBorders = iniReader.ReadInteger("MISC", "D3DHookBorders", 0) != 0;
    bool bBlackMagazineFix = iniReader.ReadInteger("MISC", "BlackMagazineFix", 0) != 0;
    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    if (!Screen.nWidth || !Screen.nHeight)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //Game state
    nGameState = std::ref(**hook::get_pattern<int32_t*>("83 3D ? ? ? ? 06 ? ? A1", 2)); //0x77A920

    //Resolution
    //menu
    auto pattern = hook::pattern("68 20 03 00 00 BE 58 02 00 00");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);

    pattern = hook::pattern("68 20 03 00 00 BE 58 02 00 00");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);

    //game
    pattern = hook::pattern("B8 20 03 00 00 BE 58 02 00 00 77 75");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    injector::MakeNOP(pattern.get_first(10), 9, true);
    pattern = hook::pattern("B8 80 02 00 00 BE E0 01 00 00 50");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    pattern = hook::pattern("B8 20 03 00 00 BE 58 02 00 00 50");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    pattern = hook::pattern("B8 00 04 00 00 BE 00 03 00 00 50");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    pattern = hook::pattern("B8 00 05 00 00 BE C0 03 00 00 50");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);

    //HUD
    if (bFixHUD)
    {
        pattern = hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8 81 00 00 00 D8"); //6CC914
        injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHudScaleX, true);

        for (size_t i = 0; i < 4; i++)
        {
            auto pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
            if (!pattern.empty())
            {
                injector::WriteMemory<float>(pattern.get(0).get<void>(7), Screen.fHudPosX, true);
            }
            else
            {
                pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
                if (!pattern.empty())
                {
                    injector::WriteMemory<float>(pattern.get(0).get<void>(4), Screen.fHudPosX, true);
                }
            }
        }

        pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43");
        injector::WriteMemory<float>(pattern.get_first(4), Screen.fHudPosX, true);
        pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43");
        injector::WriteMemory<float>(pattern.get_first(4), Screen.fHudPosX, true);

        pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC");
        injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHudPosX, true);

        pattern = hook::pattern("C7 44 24 ? 00 00 DC 43 C7 44 24 ? 00 00 70 41");
        if (!pattern.empty())
        {
            injector::WriteMemory<float>(pattern.get_first(20), (Screen.fHudPosX - 320.0f) + 200.0f, true);
            injector::WriteMemory<float>(pattern.get_first(42), (Screen.fHudPosX - 320.0f) + 200.0f, true);
            injector::WriteMemory<float>(pattern.get_first(4), (Screen.fHudPosX - 320.0f) + 440.0f, true);
            injector::WriteMemory<float>(pattern.get_first(64), (Screen.fHudPosX - 320.0f) + 440.0f, true);
        }

        //pattern = hook::pattern("8D 72 04 83 C0 60 E8 ? ? ? ? 5F 5E C3"); //menu (#179)
        //if (pattern.size() > 0)
        //  injector::MakeNOP(pattern.get_first(), 11, true);

        //mouse cursor fix
        pattern = hook::pattern("81 C2 C0 FE FF FF");
        for (size_t i = 0; i < pattern.count_hint(26).size(); ++i)
        {
            injector::WriteMemory(pattern.get(i).get<void>(2), static_cast<int32_t>(-Screen.fHudPosX), true);
        }

        pattern = hook::pattern("05 C0 FE FF FF");
        for (size_t i = 0; i < pattern.count_hint(3).size(); ++i)
        {
            injector::WriteMemory(pattern.get(i).get<void>(1), static_cast<int32_t>(-Screen.fHudPosX), true);
        }

        pattern = hook::pattern("8D 88 C0 FE FF FF");
        injector::WriteMemory(pattern.get_first(2), static_cast<int32_t>(-Screen.fHudPosX), true);

        pattern = hook::pattern("8B 06 8B 4C 24 08");
        struct ClientRectHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.esi + 0x00);
                regs.ecx = static_cast<uint32_t>(Screen.fHudPosX + Screen.fHudPosX);
            }
        }; injector::MakeInline<ClientRectHook1>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern("8B 07 8B 4C 24 0C");
        struct ClientRectHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.edi + 0x00);
                regs.ecx = 480;
            }
        }; injector::MakeInline<ClientRectHook2>(pattern.get_first(0), pattern.get_first(6));
    }

    if (bFixFOV)
    {
        static float hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        static float ver3DScale = 0.75f;
        static float mirrorScale = 0.45f;
        static float f1234 = 1.25f;
        static float f06 = 0.6f;
        static float f1 = 1.0f; // horizontal for vehicle reflection
        static float flt1 = 0.0f;
        static float flt2 = 0.0f;
        static float flt3 = 0.0f;

        if (bScaling)
        {
            hor3DScale /= 1.071428537f;
        }

        pattern = hook::pattern("DB 40 18 C7 44 24");
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x20) = 1.0f;
                regs.eax = *(uint32_t*)(regs.ebp + 4);

                if (regs.eax == 1 || regs.eax == 4) //Headlights stretching, reflections etc
                {
                    flt1 = hor3DScale;
                    flt2 = f06;
                    flt3 = f1234;
                }
                else
                {
                    if (regs.eax > 9)
                    {
                        flt1 = f1;
                        flt2 = 0.5f;
                        flt3 = 1.0f;
                        _asm fld ds : f1
                        return;
                    }
                    else
                    {
                        flt1 = 1.0f;
                        flt2 = 0.5f;
                        flt3 = 1.0f;
                    }
                }

                if (regs.eax == 3) //if rearview mirror
                    _asm fld ds : mirrorScale
                    else
                        _asm fld ds : ver3DScale
                    }
        }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(14));

        pattern = hook::pattern("D8 3D ? ? ? ? D9 1F E8 ? ? ? ? D9");
        injector::WriteMemory(pattern.get_first(2), &flt1, true);

        // FOV being different in menus and in-game fix
        pattern = hook::pattern("D8 0D ? ? ? ? ? ? E8 ? ? ? ? DD D8");
        injector::WriteMemory(pattern.get_first(2), &flt2, true);

        pattern = hook::pattern("D8 3D ? ? ? ? D9 E0 D9 5E 54 D9 44 24 20");
        injector::WriteMemory(pattern.get_first(2), &flt3, true);

        //Fixes vehicle reflection so that they're no longer broken and look exactly as they do without the widescreen fix.
        static uint16_t dx = 16400;
        pattern = hook::pattern("66 8B 15 ? ? ? ? 66 89 90 C4 00 00 00");
        injector::WriteMemory(pattern.get_first(3), &dx, true);
    }

    if (nFMVWidescreenMode)
    {
        pattern = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF");
        injector::WriteMemory<float>(pattern.get_first(6 + 0), (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Bottom)
        injector::WriteMemory<float>(pattern.get_first(6 + 5), (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
        injector::WriteMemory<float>(pattern.get_first(6 + 10), -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Top)
        injector::WriteMemory<float>(pattern.get_first(6 + 15), -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)

        if (nFMVWidescreenMode > 1)
        {
            injector::WriteMemory<float>(pattern.get_first(6 + 0), (0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Bottom)
            injector::WriteMemory<float>(pattern.get_first(6 + 5), (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
            injector::WriteMemory<float>(pattern.get_first(6 + 10), -(0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Top)
            injector::WriteMemory<float>(pattern.get_first(6 + 15), -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)
        }
    }

    if (bHUDWidescreenMode && (Screen.fAspectRatio > (4.0f / 3.0f)))
    {
        static std::vector<CDatEntry> HudCoords;

        CIniReader iniReader("");
        auto DataFilePath = iniReader.GetIniPath();
        auto pos = DataFilePath.rfind('.');
        if (pos != std::string::npos)
            DataFilePath.replace(pos, DataFilePath.length() - pos, ".dat");
        else
            DataFilePath.append(".dat");

        LoadDatFile(DataFilePath, [](std::string_view line)
        {
            int32_t PosX, PosY;
            float fOffsetX, fOffsetY;
            sscanf_s(line.data(), "%*s %x %x %f %f %*f %*f", &PosX, &PosY, &fOffsetX, &fOffsetY);

            if (Screen.fAspectRatio < (16.0f / 9.0f))
                fOffsetX /= (((16.0f / 9.0f) / Screen.fAspectRatio) * 1.5f);

            HudCoords.emplace_back(PosX, PosY, fOffsetX, fOffsetY);
        });

        static float fMinimapPosX = 320.0f;
        static float fMinimapPosY = 240.0f;

        static auto WidescreenHud = [](HudPos& HudPosX, HudPos& HudPosY, bool bAddHudOffset = false)
        {
            //if (nGameState != 3) // Disables WidescreenHUD in FrontEnd
            {
                auto it = std::find_if(HudCoords.begin(), HudCoords.end(),
                                       [cc = CDatEntry(HudPosX.dwPos, HudPosY.dwPos, 0.0f, 0.0f)]
                                       (const CDatEntry& cde) -> bool { return (cc.dwPosX == cde.dwPosX && cc.dwPosY == cde.dwPosY); });

                if (it != HudCoords.end())
                {
                    HudPosX.fPos += it->fOffsetX;
                    HudPosY.fPos += it->fOffsetY;

                    if (bAddHudOffset)
                    {
                        HudPosX.fPos -= fMinimapPosX;
                        HudPosX.fPos += Screen.fHudPosX;
                    }
                }
            }
        };

        pattern = hook::pattern("8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04");
        if (!pattern.empty())
        {
            struct HudHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = *(uint32_t*)(regs.edi + 0x1C);
                    regs.ecx = *(uint32_t*)(regs.edi + 0x20);
                    HudPos HudPosX = regs.eax;
                    HudPos HudPosY = regs.ecx;
                    WidescreenHud(HudPosX, HudPosY);
                    regs.eax = HudPosX.dwPos;
                    regs.ecx = HudPosY.dwPos;
                }
            }; injector::MakeInline<HudHook>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern("D8 05 ? ? ? ? D9 5B 68 D8 05 ? ? ? ? D9 5B 6C D9 40 3C");
            injector::WriteMemory(pattern.get_first(2), &fMinimapPosX, true);
            injector::WriteMemory(pattern.get_first(11), &fMinimapPosY, true);

            pattern = hook::pattern("8B 4D 08 D9 41 68 53 57 8B 5D 0C 8B F8");
            struct MinimapHook1
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(uint32_t*)(regs.ebp + 0x08);

                    HudPos HudPosX = *(uint32_t*)(regs.ecx + 0x68);
                    HudPos HudPosY = *(uint32_t*)(regs.ecx + 0x6C);
                    WidescreenHud(HudPosX, HudPosY, true);
                    *(uint32_t*)(regs.ecx + 0x68) = HudPosX.dwPos;
                    *(uint32_t*)(regs.ecx + 0x6C) = HudPosY.dwPos;

                    _asm fld dword ptr[HudPosX.fPos]
                }
            }; injector::MakeInline<MinimapHook1>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern("D9 40 68 8B 4D 0C D8 60 70 53 56 8D 91 83 00 00 00 D9 54 24 34");
            struct MinimapHook2
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(uint32_t*)(regs.ebp + 0x0C);

                    HudPos HudPosX = *(uint32_t*)(regs.eax + 0x68);
                    HudPos HudPosY = *(uint32_t*)(regs.eax + 0x6C);
                    WidescreenHud(HudPosX, HudPosY, true);
                    *(uint32_t*)(regs.eax + 0x68) = HudPosX.dwPos;
                    *(uint32_t*)(regs.eax + 0x6C) = HudPosY.dwPos;
                    _asm fld dword ptr[HudPosX.fPos]
                }
            }; injector::MakeInline<MinimapHook2>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern("D9 40 1C 8B 4D 08 D9 40 20");
            struct MinimapHook3
            {
                void operator()(injector::reg_pack& regs)
                {
                    HudPos HudPosX = *(uint32_t*)(regs.eax + 0x1C);
                    HudPos HudPosY = *(uint32_t*)(regs.eax + 0x20);
                    WidescreenHud(HudPosX, HudPosY, true);
                    *(uint32_t*)(regs.eax + 0x1C) = HudPosX.dwPos;
                    *(uint32_t*)(regs.eax + 0x20) = HudPosY.dwPos;
                    regs.ecx = *(uint32_t*)(regs.ebp + 0x8);
                    _asm {fld dword ptr[HudPosX.fPos]}
                    _asm {fld dword ptr[HudPosY.fPos]}
                }
            }; injector::MakeInline<MinimapHook3>(pattern.get_first(0), pattern.get_first(9));

            pattern = hook::pattern("D8 43 1C 8B 54 24 18 8B 44 24 10 D9 1A 8B 4E 68 50 51 57");
            struct LapsHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.edx = *(uint32_t*)(regs.esp + 0x18);

                    HudPos HudPosX = *(uint32_t*)(regs.ebx + 0x1C);
                    HudPos HudPosY = *(uint32_t*)(regs.ebx + 0x20);
                    WidescreenHud(HudPosX, HudPosY);
                    *(uint32_t*)(regs.ebx + 0x1C) = HudPosX.dwPos;
                    *(uint32_t*)(regs.ebx + 0x20) = HudPosY.dwPos;
                    _asm fadd dword ptr[HudPosX.fPos]
                }
            }; injector::MakeInline<LapsHook>(pattern.get_first(0), pattern.get_first(7));
        }
    }

    if (!szCustomUserFilesDirectoryInGameDir.empty())
    {
        szCustomUserFilesDirectoryInGameDir = GetExeModulePath<std::string>() + szCustomUserFilesDirectoryInGameDir;

        auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
        {
            CreateDirectoryA(szCustomUserFilesDirectoryInGameDir.c_str(), NULL);
            strcpy(pszPath, szCustomUserFilesDirectoryInGameDir.c_str());
            return S_OK;
        };

        auto dword_41C481 = *hook::pattern("68 ? ? ? ? 6A 00 6A 00 68 23 80 00 00 6A 00 FF 15").count(1).get(0).get<uint32_t*>(16 + 2);
        auto pattern = hook::pattern(pattern_str(0xFF, 0x15, to_bytes(dword_41C481)));
        for (size_t i = 0; i < pattern.size(); i++)
        {
            injector::MakeCALL(pattern.get(i).get<void*>(0), static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
            injector::MakeNOP(pattern.get(i).get<void*>(5), 1, true);
        }
    }

    if (nImproveGamepadSupport)
    {
        pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 A1 ? ? ? ? 57");
        CreateResourceFile = (void*(*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x004482F0
        pattern = hook::pattern("56 8B F2 89 8E 98 00 00 00 8B 0D ? ? ? ? 89 86 94 00 00 00 8B 86 9C 00 00 00");
        ResourceFileBeginLoading = (void(__fastcall *)(int a1, void* rsc, int a2)) pattern.get_first(0); //0x00448110;

        if (nImproveGamepadSupport < 3)
        {
            static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length());

            if (nImproveGamepadSupport == 1)
                TPKPath += "buttons-xbox.tpk";
            else if (nImproveGamepadSupport == 2)
                TPKPath += "buttons-playstation.tpk";

            static injector::hook_back<void(__cdecl*)()> hb_448600;
            auto LoadTPK = []()
            {
                LoadResourceFile(TPKPath.c_str(), 1);
                return hb_448600.fun();
            };

            pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 6A 00 6A 00 6A 00 68 F0 59 00 00"); //0x447280
            hb_448600.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();
        }

        struct PadState
        {
            int32_t LSAxis1;
            int32_t LSAxis2;
            int32_t LTRT;
            int32_t RSAxis1;
            int32_t RSAxis2;
            uint8_t unk[28];
            int8_t A;
            int8_t B;
            int8_t X;
            int8_t Y;
            int8_t LB;
            int8_t RB;
            int8_t Select;
            int8_t Start;
            int8_t LSClick;
            int8_t RSClick;
        };

        static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate;
        pattern = hook::pattern("7C ? 5F 5D 5E 33 C0 5B C2 08 00"); //0x41989E
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(8 + 5), 0x900008C2, true);
        //static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("").count(1).get(0).get<uint32_t*>(8); //0x
        struct CatchPad
        {
            void operator()(injector::reg_pack& regs)
            {
                PadState* PadKeyPresses = *(PadState**)(regs.esp + 0x4);
                //Keyboard
                //006F9358 backspace
                //006F931C enter
                if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1 && nGameState == 3)
                {
                    if (PadKeyPresses->LSClick && PadKeyPresses->RSClick)
                    {
                        if (!Qstate)
                        {
                            keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Qstate = true;
                    }
                    else
                        Qstate = false;

                    if (PadKeyPresses->Y)
                    {
                        if (!Zstate)
                        {
                            keybd_event(BYTE(VkKeyScan('Z')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('Z')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Zstate = true;
                    }
                    else
                        Zstate = false;

                    if (PadKeyPresses->X)
                    {
                        if (!Pstate)
                        {
                            keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Pstate = true;
                    }
                    else
                        Pstate = false;

                    if (PadKeyPresses->LSClick && !PadKeyPresses->RSClick)
                    {
                        if (!Tstate)
                        {
                            keybd_event(BYTE(VkKeyScan('T')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('T')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Tstate = true;
                    }
                    else
                        Tstate = false;

                    if (PadKeyPresses->RSClick && !PadKeyPresses->LSClick)
                    {
                        if (!Dstate)
                        {
                            keybd_event(BYTE(VkKeyScan('D')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('D')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Dstate = true;
                    }
                    else
                        Dstate = false;

                    if (PadKeyPresses->Select)
                    {
                        if (!Cstate)
                        {
                            keybd_event(BYTE(VkKeyScan('C')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('C')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Cstate = true;
                    }
                    else
                        Cstate = false;
                }
            }
        }; injector::MakeInline<CatchPad>(pattern.get_first(8));

        const wchar_t* ControlsTexts[] = { L" 0", L" 1", L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L" Up", L" Down", L" Left", L" Right", L"X Rotation", L"Y Rotation", L"X Axis", L"Y Axis", L"Z Axis", L"Hat Switch" };
        const wchar_t* ControlsTextsXBOX[] = { L"A", L"B", L"X", L"Y", L"LB", L"RB", L"View (Select)", L"Menu (Start)", L"Left stick", L"Right stick", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"LT / RT", L"D-pad" };
        const wchar_t* ControlsTextsPS[] = { L"Cross", L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"Select", L"Start", L"L3", L"R3", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"L2 / R2", L"D-pad" };

        static std::vector<std::wstring> Texts(ControlsTexts, std::end(ControlsTexts));
        static std::vector<std::wstring> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
        static std::vector<std::wstring> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

        pattern = hook::pattern("8D 43 60 8D 74 24 10 E8 ? ? ? ? 8B"); //0x4148F6
        struct Buttons
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = regs.ebx + 0x60;
                regs.esi = regs.esp + 0x10;

                auto pszStr = (wchar_t*)(regs.esp + 0x10);
                auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::wstring& str) { std::wstring s(pszStr); return s.find(str) != std::wstring::npos; });
                auto i = std::distance(Texts.begin(), it);

                if (it != Texts.end())
                {
                    if (nImproveGamepadSupport != 2)
                        wcscpy(pszStr, TextsXBOX[i].c_str());
                    else
                        wcscpy(pszStr, TextsPS[i].c_str());
                }
            }
        }; if (pattern.size() > 0) { injector::MakeInline<Buttons>(pattern.get_first(0), pattern.get_first(7)); }

        // FrontEnd button remap (through game code, not key emulation)
        auto pattern = hook::pattern("8B 86 ? ? ? ? 8B 0C ? ? ? ? ? 85 C9 0F"); // 004071C5
        struct FrontEndRemap
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uintptr_t*)(regs.esi + 0x0130);

                int LB = (regs.esi + 0x0130) - (0x88);
                *(uintptr_t*)LB = 0x1A; // FE Action "Comma"
                int RB = (regs.esi + 0x0130) - (0x84);
                *(uintptr_t*)RB = 0x19; // FE Action "Period"
            }
        };
        injector::MakeInline<FrontEndRemap>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
    }

    if (fLeftStickDeadzone)
    {
        // [ -10000 | 10000 ]
        static int32_t nLeftStickDeadzone = static_cast<int32_t>(fLeftStickDeadzone * 100.0f);
        pattern = hook::pattern("85 F6 7D 3D 8B 7C 24 18 57"); //0x4193B6
        static auto loc_4193F7 = (uint32_t)hook::get_pattern("83 7C 24 18 01 75 ? B9 04 00 00 00");
        static auto dword_71D8A8 = *hook::get_pattern<int32_t*>("8D 95 ? ? ? ? 8B F2 52 B9", 2);
        struct DeadzoneHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = regs.eax;
                if (*(int32_t*)&regs.esi >= 0)
                {
                    int32_t dStickStateX = *(int32_t*)(dword_71D8A8 + 0);
                    int32_t dStickStateY = *(int32_t*)(dword_71D8A8 + 1);

                    *(int32_t*)(dword_71D8A8 + 0) = (std::abs(dStickStateX) <= nLeftStickDeadzone) ? 0 : dStickStateX;
                    *(int32_t*)(dword_71D8A8 + 1) = (std::abs(dStickStateY) <= nLeftStickDeadzone) ? 0 : dStickStateY;

                    *(uint32_t*)(regs.esp - 4) = loc_4193F7;
                }
            }
        }; injector::MakeInline<DeadzoneHook>(pattern.get_first(-2), pattern.get_first(4));
    }

    if (bD3DHookBorders)
    {
        pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 68 1F 00 08 00 6A 03 E8");
        static auto dword_736368 = *pattern.get_first<IDirect3D9**>(1);
        struct Direct3DDeviceHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto pID3D9 = (IDirect3D9*)regs.eax;
                *dword_736368 = pID3D9;
                UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)pID3D9));
                RealD3D9CreateDevice = (CreateDevice_t)pVTable[IDirect3D9VTBL::CreateDevice];
                injector::WriteMemory(&pVTable[IDirect3D9VTBL::CreateDevice], &CreateDevice, true);
            }
        }; injector::MakeInline<Direct3DDeviceHook>(pattern.get_first(0), pattern.get_first(5)); //40883C
    }

    if (bBlackMagazineFix)
    {
        pattern = hook::pattern("8B F8 A0 ? ? ? ? 84 C0 0F 85");
        byte_735768 = *pattern.get_first<uint8_t*>(3);

        pattern = hook::pattern("83 3D ? ? ? ? 01 8B 77 58");
        dword_73645C = *pattern.get_first<uint32_t*>(2);
        injector::MakeRangedNOP(pattern.get_first(0), pattern.get_first(7));
        injector::MakeCALL(pattern.get_first(0), CmpShouldClearSurfaceDuring3DRender); // 4098A2

        pattern = hook::pattern("6A 00 BE ? ? ? ? 74 ? BE");
        off_71AA4C = *pattern.get_first<void*>(10);
        injector::MakeCALL(pattern.get_first(9), MaybeClearSurfaceDuringBackgroundRender); // 409CF2
    }

    //__mbclen to strlen, "---" bug fix
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 76 21 8D 43 60");
    if (!pattern.empty())
    {
        injector::MakeCALL(pattern.get_first(0), strlen, true);
    }

    //icon fix
    auto SetWindowLongHook = [](HWND hWndl, int nIndex, LONG dwNewLong)
    {
        SetWindowLongA(hWndl, nIndex, dwNewLong);
        SendMessage(hWndl, WM_SETICON, ICON_BIG, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)));
        SendMessage(hWndl, WM_SETICON, ICON_SMALL, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)));
    };

    pattern = hook::pattern("FF 15 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 33 D2 3B CB 0F 95 C2");
    injector::MakeNOP(pattern.get_first(0), 6, true);
    injector::MakeCALL(pattern.get_first(0), static_cast<void(WINAPI*)(HWND, int, LONG)>(SetWindowLongHook), true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("68 20 03 00 00 BE 58 02 00 00").count_hint(1).empty(), 0x1100);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, uint32_t reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}