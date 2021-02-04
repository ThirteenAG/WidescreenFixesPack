#include "stdafx.h"
#include "cxbxr/cxbxr.h"

struct cxbxr
{
    uintptr_t begin;
    uintptr_t end;
} cxbxr;

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
            cxbxr.begin = (uintptr_t)GetModuleHandle(nullptr);
            PIMAGE_DOS_HEADER dosHeader = (IMAGE_DOS_HEADER*)(cxbxr.begin + 0);
            PIMAGE_NT_HEADERS ntHeader = (IMAGE_NT_HEADERS*)(cxbxr.begin + dosHeader->e_lfanew);
            cxbxr.end = cxbxr.begin + ntHeader->OptionalHeader.SizeOfImage;

            CallbackHandler::RegisterCallback(Init, hook::range_pattern(cxbxr.begin, cxbxr.end, "8B 0D ? ? ? ? A1 ? ? ? ? 2B C1 05"));
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