#include "stdafx.h"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fConditionalAspect;
    float fHudScale;
    float fHudZoomScale;
    float ShadowFix;
} Screen;

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    Screen.fConditionalAspect = iniReader.ReadFloat("MAIN", "ConditionalHorizontalAspect", 4.0f / 3.0f);
    Screen.fHudZoomScale = iniReader.ReadFloat("MAIN", "ZoomFactor", 1.0f);

    if (!Screen.Width || !Screen.Height)
    	std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    if (Screen.fAspectRatio < Screen.fConditionalAspect)
    	Screen.fHudZoomScale = Screen.fHudZoomScale * (Screen.fAspectRatio / Screen.fConditionalAspect);
    Screen.fHudScale = ((Screen.fHudZoomScale / Screen.fAspectRatio) * (4.0f / 3.0f));

    //446B2A
    auto pattern = hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = Screen.Width;
            regs.ecx = Screen.Height;
        }
    }; injector::MakeInline<ResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(27));

    pattern = hook::pattern("74 ? 8B 4C 24 04 8B 54 24 08 89 0D"); //0x446B06
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

    pattern = hook::pattern("A3 ? ? ? ? 8B 42 08 A3 ? ? ? ? 8B"); //655123
    static auto pResY = *pattern.count(1).get(0).get<uint32_t*>(1);
    struct ResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(pResY - 1) = Screen.Width;
            *(pResY - 0) = Screen.Height;
        }
    }; injector::MakeInline<ResHook2>(pattern.count(1).get(0).get<uint32_t>(0));

    pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 68 ? ? ? ? 50 8B 08"); //6557A8
    struct ResHook2A
    {
        void operator()(injector::reg_pack& regs)
        {
            *(pResY - 1) = Screen.Width;
            *(pResY - 0) = Screen.Height;
        }
    }; injector::MakeInline<ResHook2A>(pattern.count(1).get(0).get<uint32_t>(0));

    pattern = hook::pattern("89 0D ? ? ? ? 8B 6C 24 7C 8B 75 60 80 7E 20 05"); //657047
    struct ResHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            *(pResY - 1) = Screen.Width;
            *(pResY - 0) = Screen.Height;
        }
    }; injector::MakeInline<ResHook3>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

    pattern = hook::pattern("89 15 ? ? ? ? 8B 51 08 89 15 ? ? ? ? 8B 49 0C"); //
    struct ResHook4
    {
        void operator()(injector::reg_pack& regs)
        {
            *(pResY - 1) = Screen.Width;
            *(pResY - 0) = Screen.Height;
        }
    }; injector::MakeInline<ResHook4>(pattern.count(3).get(2).get<uint32_t>(0), pattern.count(3).get(2).get<uint32_t>(6));

    pattern = hook::pattern("89 0D ? ? ? ? 5F 5E 5D B8 01 00 00 00"); //657CAF
    struct ResHook5
    {
        void operator()(injector::reg_pack& regs)
        {
            *(pResY - 1) = Screen.Width;
            *(pResY - 0) = Screen.Height;
        }
    }; injector::MakeInline<ResHook5>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

    pattern = hook::pattern("D9 42 68 D8 08 D9 42 68 D8 48 04 D9 42 68 D8 48 08"); //0x64AFDC
    struct CutOffAreaHORHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float edx68 = *(float*)(regs.edx + 0x68) / Screen.fHudScale;
            float edx6C = *(float*)(regs.edx + 0x6C) / Screen.fHudZoomScale;
            float eax00 = *(float*)(regs.eax + 0);
            float eax04 = *(float*)(regs.eax + 4);
            float eax08 = *(float*)(regs.eax + 8);
            _asm
            {
                fld     dword ptr[edx68]
                fmul    dword ptr[eax00]
                fld     dword ptr[edx68]
                fmul    dword ptr[eax04]
                fld     dword ptr[edx68]
                fmul    dword ptr[eax08]
            }
        }
    }; injector::MakeInline<CutOffAreaHORHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(17));
    
    struct CutOffAreaVertHook
	{
		void operator()(injector::reg_pack& regs)
		{
            float edx6C = *(float*)(regs.edx + 0x6C) / Screen.fHudZoomScale;
            float eax10 = *(float*)(regs.eax + 0x10);
            _asm
            {
                fld     dword ptr[edx6C]
                fmul    dword ptr[eax10]
            }
         }
    }; injector::MakeInline<CutOffAreaVertHook>(pattern.count(1).get(0).get<uint32_t>(42), pattern.count(1).get(0).get<uint32_t>(6));
    
    pattern = hook::pattern("83 C3 0C 53 56 E8 ?? ?? ?? ?? ??"); //0x6B7272
    struct ShadowFixHook
	{
		void operator()(injector::reg_pack& regs)
		{
            float ebx00 = *(float*)(regs.ebx + 0x00);
            float ebx04 = *(float*)(regs.ebx + 0x04);
            _asm
            {
                add ebx,0x0C
                fld     dword ptr[ebx00]
                fmul    dword ptr[Screen.fHudScale]
                fstp    dword ptr[ShadowFix]
                fld     dword ptr[ebx04]
                fmul    dword ptr[Screen.fHudZoomScale]
                fstp    dword ptr[ShadowFix + 0x4]
                mov ebx, ShadowFix
                push ebx
                push esi
            }
         }
    }; injector::MakeInline<ShadowFixHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));
    
    pattern = hook::pattern("D9 05 ? ? ? ? 89 4E 68 8B 50 04 D8 76 68"); //0x64AC8B
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.fHudScale, true);
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(28), &Screen.fHudZoomScale, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B"));
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
