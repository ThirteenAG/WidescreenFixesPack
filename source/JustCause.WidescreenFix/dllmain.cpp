#include "stdafx.h"
#include <d3d9.h>
#include "dxsdk\d3dvtbl.h"

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
    bool bDrawBorders;
    float fWidescreenOffsetX;
    float fWidescreenOffsetY;
    bool bFullscreenFMVs;
    bool bExtraBorders;
} Screen;

struct testParam
{
    float a1;
    float a2;
    float a3;
    float a4;
    float a5;
    float a6;
    float a7;
    float a8;
    float a9;
    float a10;
    float a11;
    float a12;
};

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
    if (Screen.bDrawBorders)
    {
        if (Screen.bFullscreenFMVs)
        {
            DrawRect(pDevice, 0, 0, static_cast<int32_t>(Screen.fWidescreenOffsetX), Screen.nHeight);
            DrawRect(pDevice, static_cast<int32_t>(Screen.fWidth - Screen.fWidescreenOffsetX), 0, static_cast<int32_t>(Screen.fWidth - Screen.fWidescreenOffsetX - Screen.fWidescreenOffsetX), Screen.nHeight);
        }
        else
        {
            DrawRect(pDevice, 0, 0, static_cast<int32_t>(Screen.fHudOffsetReal), Screen.nHeight);
            DrawRect(pDevice, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal), 0, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal + Screen.fHudOffsetReal), Screen.nHeight);
        }
        //Screen.bDrawBorders = false;
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

injector::hook_back<void(__fastcall*)(void*, void*, testParam*)> hbDrawBorders;
void __fastcall DrawBordersHook(void* _this, void* edx, testParam* a2)
{
    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("{0:f} {1:f} {2:f} {3:f} {4:f} {5:f} {6:f} {7:f} {8:f}", a2->a1, a2->a2, a2->a3, a2->a4, a2->a5, a2->a6, a2->a7, a2->a8, a2->a9); });

    static float x4 = 0.0f;
    if (!x4 && (fabs(-a2->a2 - a2->a4) <= FLT_EPSILON * fmax(fabs(-a2->a2), fabs(a2->a4))))
        x4 = a2->a4;

    if (*(int32_t*)&a2->a1 == *(int32_t*)&a2->a3)
    {
        Screen.bDrawBorders = true;

        if (Screen.bFullscreenFMVs)
        {
            //if (a2->a4 == 81.0f || a2->a4 == 40.5f) //cutscene borders
            if (a2->a4 == x4)
            {
                Screen.bDrawBorders = false;
                a2->a1 = 0.0f;
                a2->a2 = 0.0f;
                a2->a3 = 0.0f;
                a2->a4 = 0.0f;
                return hbDrawBorders.fun(_this, edx, a2);
            }

            a2->a3 -= Screen.fWidescreenOffsetX;
            a2->a3 *= Screen.fHudScale;
            a2->a4 += Screen.fWidescreenOffsetY;
        }
    }

    //if (a2->a4 != 81.0f && a2->a4 != 40.5f) //cutscene borders
    if (a2->a4 == x4)
    {
        Screen.bDrawBorders = false;
    }
    else
    {
        a2->a1 /= Screen.fHudScale;
        a2->a3 /= Screen.fHudScale;
        a2->a1 += Screen.fHudOffsetReal;
    }

    hbDrawBorders.fun(_this, edx, a2);
}

void Init()
{
    CIniReader iniReader("");
    Screen.nWidth = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.nHeight = iniReader.ReadInteger("MAIN", "ResY", 0);
    Screen.bFullscreenFMVs = iniReader.ReadInteger("MAIN", "FullscreenFMVs", 0) != 0;
    Screen.bExtraBorders = iniReader.ReadInteger("MAIN", "ExtraBorders", 1) != 0;

    if (!Screen.nWidth || !Screen.nHeight)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

    int32_t defX = 0, defY = 0;
    std::tie(defX, defY) = GetDesktopRes();
    auto pattern = hook::pattern("C7 46 08 ? ? ? ? C7 46 0C ? ? ? ? 88 5E 18");
    injector::WriteMemory(pattern.get_first(3), defX, true); //497822
    injector::WriteMemory(pattern.get_first(10), defY, true);

    pattern = hook::pattern("77 ? FF 24 85 ? ? ? ? C7 46");
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEBi8, true); //jmp
    pattern = hook::pattern("C7 46 1C 15 00 00 00");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esi + 0x1C) = 0x15;
            *(int32_t*)(regs.esi + 0x08) = Screen.nWidth;
            *(int32_t*)(regs.esi + 0x0C) = Screen.nHeight;

            Screen.fWidth = static_cast<float>(Screen.nWidth);
            Screen.fHeight = static_cast<float>(Screen.nHeight);
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fWidescreenOffsetX = (Screen.fWidth - Screen.fHeight * (16.0f / 9.0f)) / 2.0f;
            Screen.fWidescreenOffsetY = (Screen.fHeight - (Screen.fHeight * ((4.0f / 3.0f) / (16.0f / 9.0f)))) / 2.0f;
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(7)); //0x40417C

    //Screen "bleeds" and flashes a lot with properly scaled backgrounds, so I need pillarboxing to hide that
    pattern = hook::pattern("8D 85 90 FE FF FF 50 6A 01 6A 00");
    struct Direct3DDeviceHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto pID3D9 = (IDirect3D9*)regs.eax;
            UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)pID3D9));
            RealD3D9CreateDevice = (CreateDevice_t)pVTable[IDirect3D9VTBL::CreateDevice];
            injector::WriteMemory(&pVTable[IDirect3D9VTBL::CreateDevice], &CreateDevice, true);
            regs.eax = regs.ebp - 0x170;
        }
    }; injector::MakeInline<Direct3DDeviceHook>(pattern.get_first(0), pattern.get_first(6)); //0x48A480, 0x48A480+6

    //Same hook used to scale hud
    pattern = hook::pattern("E8 ? ? ? ? 8D 96 90 00 00 00 85 D2");
    hbDrawBorders.fun = injector::MakeCALL(pattern.get_first(0), DrawBordersHook).get(); //0x509340

    //Additional hook for pillaboxing during FMVs
    pattern = hook::pattern("C7 45 F8 00 00 00 00 83 3D ? ? ? ? 00 0F 84");
    struct BinkBordersHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebp - 0x08) = 0;
            Screen.bDrawBorders = true;
        }
    }; injector::MakeInline<BinkBordersHook>(pattern.get_first(0), pattern.get_first(7)); // 0x46A5A2, 0x46A5A2 + 7

    auto sub_480BA0 = [](uintptr_t _this, uint32_t edx, int32_t* x, int32_t* y)
    {
        *x = Screen.nWidth43; //*(int16_t*)(_this + 0x0C);
        *y = *(int16_t*)(_this + 0x0E);
        Screen.bDrawBorders = false;
    };

    //some part of the hud is not stretched, so I need to rescale it
    pattern = hook::pattern("E8 ? ? ? ? DB 45 F0 DA 75 E8 D9 5D EC 8B 0D"); //0x685ED3
    injector::MakeCALL(pattern.get_first(0), static_cast<void(__fastcall *)(uintptr_t, uint32_t, int32_t*, int32_t*)>(sub_480BA0), true);

    //radar is scaled separately
    pattern = hook::pattern("E8 ? ? ? ? DB 45 E4 DA 75 E8 D9 5D FC 8B 4D 94"); //0x7534CA
    injector::MakeCALL(pattern.get_first(0), static_cast<void(__fastcall *)(uintptr_t, uint32_t, int32_t*, int32_t*)>(sub_480BA0), true); //radar
    pattern = hook::pattern("E8 ? ? ? ? DB 44 24 0C 8B 86 04 01"); //0x8AD11F
    injector::MakeCALL(pattern.get_first(0), static_cast<void(__fastcall *)(uintptr_t, uint32_t, int32_t*, int32_t*)>(sub_480BA0), true); //radar

    if (!Screen.bExtraBorders)
    {
        //when no hud on screen, pillarboxes appear
        auto sub_480BA0_2 = [](uintptr_t _this, uint32_t edx, int32_t* x, int32_t* y)
        {
            *x = *(int16_t*)(_this + 0x0C);
            *y = *(int16_t*)(_this + 0x0E);
            Screen.bDrawBorders = false;
        };
        pattern = hook::pattern("E8 ? ? ? ? DB 44 24 08 8A 86"); //0x5092D9
        injector::MakeCALL(pattern.get_first(0), static_cast<void(__fastcall *)(uintptr_t, uint32_t, int32_t*, int32_t*)>(sub_480BA0_2), true); //radar
    }

    //pda map is broken now
    pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 44 7A ? C7 85 30 FF FF FF 01 00 00 00 EB");
    static const float f1 = 1.0f; //map fix
    injector::WriteMemory(pattern.get_first(2), &f1, true); // 0x752E83 + 2
    pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 44 7A ? C7 85 FC FD FF FF 01 00 00 00 EB");
    injector::WriteMemory(pattern.get_first(2), &f1, true); // 0x751DCB + 2

    pattern = hook::pattern("8B 95 08 FE FF FF D8 7A 2C D9 5D 94");
    struct RadarHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.ebp - 0x68) = (4.0f / 3.0f);
            regs.edx = *(uint32_t*)(regs.ebp - 0x1F8);
        }
    }; injector::MakeInline<RadarHook1>(pattern.get_first(0), pattern.get_first(6)); //0x752225, 0x752225+6

    pattern = hook::pattern("8B 95 3C FF FF FF 0F B6 42 30 85 C0 74 ? 8B 4D 08");
    struct RadarHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.ebp - 0x4) = (4.0f / 3.0f);
            regs.edx = *(uint32_t*)(regs.ebp - 0xC4);
        }
    }; injector::MakeInline<RadarHook2>(pattern.get_first(0), pattern.get_first(6)); //0x752E0E, 0x752E0E + 6

    pattern = hook::pattern("8B 8E E8 00 00 00 D9 5C 24 54 D8 4C 24 0C D9 5C 24 58");
    struct RadarHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.esi + 0xE8);

            *(float*)(regs.esp + 0x18) += Screen.fHudOffsetReal;
            *(float*)(regs.esp + 0x4C) += Screen.fHudOffsetReal;
        }
    }; injector::MakeInline<RadarHook3>(pattern.get_first(0), pattern.get_first(6)); //0x8AD1EB, 0x8AD1EB+6

    //Text
    pattern = hook::pattern("8B 48 04 8B 86 88 00 00 00 C1 E8 0D 83 E0 0F 83 E8 00");
    struct TextScaleHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.nWidth43;
            regs.eax = *(int32_t*)(regs.esi + 0x88);
        }
    }; injector::MakeInline<TextScaleHook1>(pattern.get_first(0), pattern.get_first(9)); //0x5D079F, 0x5D079F + 9

    struct TextScaleHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.nWidth43;
            regs.edx = *(int32_t*)(regs.eax + 0x08);
        }
    };
    pattern = hook::pattern("8B 48 04 8B 50 08 89 4C 24 44 89 54 24 40");
    injector::MakeInline<TextScaleHook2>(pattern.get_first(0), pattern.get_first(6)); //0x5D10AF, 0x5D10AF + 6
    pattern = hook::pattern("8B 48 04 8B 50 08 89 4C 24 34 89 54 24 2C");
    injector::MakeInline<TextScaleHook2>(pattern.get_first(0), pattern.get_first(6)); //0x5D1563, 0x5D1563 + 6
    pattern = hook::pattern("8B 48 04 8B 50 08 8B 86 88");
    injector::MakeInline<TextScaleHook2>(pattern.get_first(0), pattern.get_first(6)); //0x5D269B, 0x5D269B + 6
    //injector::MakeInline<TextScaleHook2>(); //0x5D2337, 0x5D2337+6 //crash

    pattern = hook::pattern("8B 48 04 8B BE A8 00 00 00");
    struct TextScaleHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.nWidth43;
            regs.edi = *(int32_t*)(regs.esi + 0xA8);
        }
    }; injector::MakeInline<TextScaleHook3>(pattern.get_first(0), pattern.get_first(9)); //0x5D29FB, 0x5D29FB + 9

    pattern = hook::pattern("8B 8E 20 01 00 00 D8 0D ? ? ? ? 89 4C 24 20");
    struct TextPosHook1 //subs
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x0C) += Screen.fHudOffsetReal;
            regs.ecx = *(uint32_t*)(regs.esi + 0x120);
        }
    }; injector::MakeInline<TextPosHook1>(pattern.get_first(0), pattern.get_first(6)); //0x5D0845, 0x5D0845 + 6

    pattern = hook::pattern("D9 44 24 0C D8 74 24 10 D8 46 0C");
    struct TextPosHook2 //subs backgr
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x24) = ((*(float*)(regs.esp + 0x0C) - Screen.fHudOffsetReal) / *(float*)(regs.esp + 0x10)) + *(float*)(regs.esi + 0x0C);
        }
    }; injector::MakeInline<TextPosHook2>(pattern.get_first(0), pattern.get_first(15)); // 0x5D086F, 0x5D086F + 15

    pattern = hook::pattern("8B 9E 20 01 00 00 8B C7");
    struct TextPosHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x0C) += Screen.fHudOffsetReal;
            regs.ebx = *(uint32_t*)(regs.esi + 0x120);
        }
    }; injector::MakeInline<TextPosHook3>(pattern.get_first(0), pattern.get_first(6)); //0x5D273F, 0x5D273F + 6

    //FMVs broken aspect ratio, someone screwed up and put 1280x780 instead of 1280x720!
    pattern = hook::pattern("C7 41 50 ? ? ? ? 68 E0 00 00 00");
    injector::WriteMemory(pattern.get_first(3), 720, true); //0x4699C8
    pattern = hook::pattern("C7 45 EC ? ? ? ? 68 ? ? ? ? E8");
    injector::WriteMemory(pattern.get_first(3), 720, true); //0x46951C

    //shadow fix
    pattern = hook::pattern("85 C9 ? 1F 6A 10");
    injector::WriteMemory<uint8_t>(pattern.get_first(2), 0xEBi8, true); //jmp
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("C7 46 08 ? ? ? ? C7 46 0C ? ? ? ? 88 5E 18"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}