#include "stdafx.h"
#include "cxbxr/cxbxr.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScale;
} Screen;

void Init()
{
    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudScale = Screen.fAspectRatio / (4.0f / 3.0f);

    struct AspectRatioHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.ebx + 0x1A4) *= Screen.fHudScale;
            regs.eax = *(uint32_t*)(regs.ebx + 0x3E0);
        }
    }; injector::MakeInline<AspectRatioHook>(0x23DFC3, 0x23DFC3 + 6);


}

CEXP void InitializeASI()
{
    auto pid = getParentPID(GetCurrentProcessId());
    if (getProcessName(pid).stem() == "cxbx")
    {
        std::tie(Screen.Width, Screen.Height) = getWindowDimensions(pid);
        std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("8B 0D ? ? ? ? A1 ? ? ? ? 2B C1 05"));
        });
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
        {
            InitializeASI();
        }
    }
    return TRUE;
}