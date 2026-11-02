module;
#include <stdafx.h>
#include <d3d9.h>
#include "d3dvtbl.h"
#include "FixSettings.hpp"

export module Misc;
import ComVars;
import Resolution;
import Imports;
import Hud;
import Video;
import Bars;
import Camera;
import Window;
import DeveloperOverlay;
import Updates;

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
using Dimensions_t = void(__fastcall*)(uintptr_t, uint32_t, int32_t*, int32_t*);
Dimensions_t NativeHiddenHudDimensions = nullptr;

HRESULT __stdcall CreateDevice(IDirect3D9* d3ddev, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
    JCWindow::Parameters(pPresentationParameters, hFocusWindow, false);
    HRESULT retval = RealD3D9CreateDevice(d3ddev, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    if (FAILED(retval) || !ppReturnedDeviceInterface || !*ppReturnedDeviceInterface)
        return retval;

    JCWindow::Attach(*ppReturnedDeviceInterface, pPresentationParameters);
    return retval;
}

injector::hook_back<void(__fastcall*)(void*, void*, testParam*)> hbDrawBorders;
void __fastcall DrawBordersHook(void* _this, void* edx, testParam* a2)
{
    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("{0:f} {1:f} {2:f} {3:f} {4:f} {5:f} {6:f} {7:f} {8:f}", a2->a1, a2->a2, a2->a3, a2->a4, a2->a5, a2->a6, a2->a7, a2->a8, a2->a9); });

    JCVideo::Quad movieQuad{};
    std::memcpy(&movieQuad, a2, sizeof(movieQuad));
    if (JCVideo::Apply(_this, movieQuad, Screen.nWidth, Screen.nHeight))
    {
        Screen.bDrawBorders = false;
        return hbDrawBorders.fun(_this, edx, reinterpret_cast<testParam*>(&movieQuad));
    }
    if (std::abs(a2->a1 - a2->a3) < 0.5f && std::abs(a2->a3 * 2.0f - Screen.fWidth) < 1.0f)
        Screen.bBoxedBackground = true;

    static float x4 = 0.0f;
    if (!x4 && (fabs(-a2->a2 - a2->a4) <= FLT_EPSILON * fmax(fabs(-a2->a2), fabs(a2->a4))))
        x4 = a2->a4;
    if (*(int32_t*)&a2->a1 == *(int32_t*)&a2->a3)
        Screen.bDrawBorders = true;

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

// Startup logo videos: opening them fails, which the game treats as a finished video.
SafetyHookInline shBinkOpen, shBinkClose;
void __stdcall BinkCloseHook(void* handle)
{
    JCVideo::activeSprite.store(nullptr);
    if (JCVideo::handle.load() == handle)
    {
        JCVideo::handle.store(nullptr);
    }
    shBinkClose.stdcall<void>(handle);
}
void* __stdcall BinkOpenHook(const char* name, uint32_t flags)
{
    constexpr uint32_t BINKFILEHANDLE = 0x00800000;
    if (FixSettings().Bool("MAIN.SkipIntro") && name && !(flags & BINKFILEHANDLE))
    {
        std::string_view file{ name };
        if (auto slash = file.find_last_of("\\/"); slash != file.npos)
            file.remove_prefix(slash + 1);
        file = file.substr(0, file.find('.'));
        for (auto logo : { "AvalancheVideo", "EidosVideo" })
        {
            if (file.size() == std::strlen(logo) && std::equal(file.begin(), file.end(), logo, [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }))
                return nullptr;
        }
    }
    const auto result = shBinkOpen.stdcall<void*>(name, flags);
    if (result)
    {
        const auto dimensions = static_cast<const uint32_t*>(result);
        if (dimensions[0] > 0 && dimensions[0] <= 16384 && dimensions[1] > 0 && dimensions[1] <= 16384)
        {
            JCVideo::handle.store(result);
            JCVideo::width.store(dimensions[0]);
            JCVideo::height.store(dimensions[1]);
        }
    }
    return result;
}

void InstallLogoSkip()
{
    if (shBinkOpen)
        return;
    if (auto bink = GetModuleHandleW(L"binkw32.dll"))
    {
        if (auto open = GetProcAddress(bink, "_BinkOpen@8"))
            shBinkOpen = safetyhook::create_inline(open, BinkOpenHook);
        if (auto close = GetProcAddress(bink, "_BinkClose@4"))
            shBinkClose = safetyhook::create_inline(close, BinkCloseHook);
        JCVideo::decoderReady = shBinkOpen && shBinkClose;
    }
}

void InitializeGame()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    InstallLogoSkip();
    JCHud::Install();
    JCWindow::Install();
    JCCamera::Install();
    JCOverlay::Install();
    JCUpdates::Init();
    Screen.nWidth = settings.Int("MAIN.ResX");
    Screen.nHeight = settings.Int("MAIN.ResY");

    if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 16384 || Screen.nHeight > 16384)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0)
        std::tie(Screen.nWidth, Screen.nHeight) = std::make_tuple(640, 480);

    int32_t defX = 0, defY = 0;
    std::tie(defX, defY) = GetDesktopRes();
    pattern = hook::pattern("C7 46 08 ? ? ? ? C7 46 0C ? ? ? ? 88 5E 18");
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(3), defX, true); //497822
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(10), defY, true);

    pattern = hook::pattern("77 ? FF 24 85 ? ? ? ? C7 46");
    injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(0), 0xEB, true); //jmp
    pattern = hook::pattern("C7 46 1C 15 00 00 00");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esi + 0x1C) = 0x15;
            *(int32_t*)(regs.esi + 0x08) = Screen.nWidth;
            *(int32_t*)(regs.esi + 0x0C) = Screen.nHeight;

            UpdateScreen(Screen.nWidth, Screen.nHeight);
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(7)); //0x40417C

    //Screen "bleeds" and flashes a lot with properly scaled backgrounds, so I need pillarboxing to hide that
    pattern = hook::pattern("8D 85 90 FE FF FF 50 6A 01 6A 00");
    struct Direct3DDeviceHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto pID3D9 = (IDirect3D9*)regs.eax;
            if (pID3D9)
            {
                auto pVTable = *reinterpret_cast<UINT_PTR**>(pID3D9);
                auto currentCreateDevice = reinterpret_cast<CreateDevice_t>(pVTable[IDirect3D9VTBL::CreateDevice]);
                if (currentCreateDevice != &CreateDevice)
                {
                    RealD3D9CreateDevice = currentCreateDevice;
                    injector::WriteMemory<CreateDevice_t>(reinterpret_cast<CreateDevice_t*>(&pVTable[IDirect3D9VTBL::CreateDevice]), &CreateDevice, true);
                }
            }
            regs.eax = regs.ebp - 0x170;
        }
    }; injector::MakeInline<Direct3DDeviceHook>(pattern.get_first(0), pattern.get_first(6)); //0x48A480, 0x48A480+6

    //Same hook used to scale hud
    pattern = hook::pattern("E8 ? ? ? ? 8D 96 90 00 00 00 85 D2");
    hbDrawBorders.fun = injector::MakeCALL(pattern.get_first(0), DrawBordersHook).get(); //0x509340

    JCVideo::Install();
    JCVideo::filter = JCCamera::Sprite;

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

    {
        //when no hud on screen, pillarboxes appear
        auto sub_480BA0_2 = [](uintptr_t _this, uint32_t edx, int32_t* x, int32_t* y)
        {
            NativeHiddenHudDimensions(_this, edx, x, y);
            Screen.bDrawBorders = false;
        };
        pattern = hook::pattern("E8 ? ? ? ? DB 44 24 08 8A 86"); //0x5092D9
        NativeHiddenHudDimensions = injector::MakeCALL(pattern.get_first(0), static_cast<Dimensions_t>(sub_480BA0_2), true).get();
    }

    //pda map is broken now
    pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 44 7A ? C7 85 30 FF FF FF 01 00 00 00 EB");
    static const float f1 = 1.0f; //map fix
    injector::WriteMemory<const float*>(pattern.get_first<const float*>(2), &f1, true); // 0x752E83 + 2
    pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 44 7A ? C7 85 FC FD FF FF 01 00 00 00 EB");
    injector::WriteMemory<const float*>(pattern.get_first<const float*>(2), &f1, true); // 0x751DCB + 2

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
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(3), 720, true); //0x4699C8
    pattern = hook::pattern("C7 45 EC ? ? ? ? 68 ? ? ? ? E8");
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(3), 720, true); //0x46951C

    //shadow fix
    pattern = hook::pattern("85 C9 ? 1F 6A 10");
    injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(2), 0xEB, true); //jmp

}


class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []() { InitializeGame(); };
    }
} Misc;
