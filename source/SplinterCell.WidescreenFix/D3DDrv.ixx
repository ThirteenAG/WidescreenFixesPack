module;

#include <stdafx.h>
#include <dxsdk/dx8/d3d8.h>

export module D3DDrv;

import ComVars;

void* FMVPtrY = nullptr;
void* FMVPtrX = nullptr;
D3DPRESENT_PARAMETERS* pPresentParams = nullptr;
SafetyHookInline shUD3DRenderDeviceSetRes = {};
int __fastcall UD3DRenderDeviceSetRes(void* UD3DRenderDevice, void* edx, void* UViewport, int width, int height, int a5)
{
    //if (width == 777 && height == 777)
    //{
    //    return shUD3DRenderDeviceSetRes.unsafe_fastcall<int>(UD3DRenderDevice, edx, UViewport, 800, 600, a5);
    //    //width = Screen.Width;
    //    //height = Screen.Height;
    //}

    bool bInvalidRes = true;
    std::wstring resStr = std::to_wstring(width) + L"x" + std::to_wstring(height);
    for (const auto& it : ResList)
    {
        if (resStr == it.first)
        {
            swscanf(it.second.c_str(), L"%dx%d", &width, &height);
            bInvalidRes = false;
            break;
        }
    }

    if (bInvalidRes)
    {
        width = Screen.Width;
        height = Screen.Height;
        bInvalidRes = false;
    }

    auto ret = shUD3DRenderDeviceSetRes.unsafe_fastcall<int>(UD3DRenderDevice, edx, UViewport, width, height, a5);

    Screen.Width = width;
    Screen.Height = height;
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fFMVoffsetStartX = (Screen.fWidth - 640.0f) / 2.0f;
    Screen.fFMVoffsetStartY = (Screen.fHeight - 480.0f) / 2.0f;
    Screen.fWidescreenHudOffset = ((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / Screen.fIniHudOffset));
    if (Screen.fAspectRatio < (16.0f / 9.0f))
        Screen.fWidescreenHudOffset = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
    if (Screen.fAspectRatio <= (4.0f / 3.0f))
        Screen.fWidescreenHudOffset = 0.0f;
    if (Screen.Width < 640 || Screen.Height < 480)
        return ret;

    injector::WriteMemory(FMVPtrY, Screen.Height, true);
    injector::WriteMemory(FMVPtrX, Screen.Width, true);

    CIniReader iniReader("");
    auto [DesktopResW, DesktopResH] = GetDesktopRes();
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

    return ret;
}

export void InitD3DDrv()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 50 8B CF FF 55 60"); //0x1000F72E
    FMVPtrY = pattern.get_first(1);
    FMVPtrX = pattern.get_first(6);

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 44 24 04 81 EC 80 00 00 00");
    shUD3DRenderDeviceSetRes = safetyhook::create_inline(pattern.get_first(), UD3DRenderDeviceSetRes);
    pPresentParams = *hook::module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? 33 C0 8B D9 C1 E9 02 83 E3 03").get_first<D3DPRESENT_PARAMETERS*>(1);

    //FMV
    static auto SetFMVPos = [](injector::reg_pack& regs)
    {
        if (Screen.Width >= 1280 && Screen.Height >= 960)
        {
            static constexpr auto BINKCOPY2XWH = 0x40000000L; // copy the width and height zoomed by two
            regs.eax |= BINKCOPY2XWH;

            *(int32_t*)&regs.ebp = static_cast<int32_t>((Screen.fWidth - (640.0f * 2.0f)) / 2.0f);
            *(int32_t*)&regs.ebx = static_cast<int32_t>((Screen.fHeight - (480.0f * 2.0f)) / 2.0f);
        }
        else
        {
            *(int32_t*)&regs.ebp = static_cast<int32_t>((Screen.fWidth - 640.0f) / 2.0f);
            *(int32_t*)&regs.ebx = static_cast<int32_t>((Screen.fHeight - 480.0f) / 2.0f);
        }
    };

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 4C 24 3C 8B 54 24 28");
    struct BINKHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.esp + 0x3C);
            regs.edx = *(uint32_t*)(regs.esp + 0x28);
            SetFMVPos(regs);
        }
    }; injector::MakeInline<BINKHook1>(pattern.get_first(0), pattern.get_first(8));

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 54 24 3C 8B 4C 24 2C");
    struct BINKHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.esp + 0x3C);
            regs.ecx = *(uint32_t*)(regs.esp + 0x2C);
            SetFMVPos(regs);
        }
    }; injector::MakeInline<BINKHook2>(pattern.get_first(0), pattern.get_first(8));
}
