module;

#include <stdafx.h>
#include <dxsdk/dx8/d3d8.h>

export module D3DDrv;

import ComVars;

export void InitD3DDrv()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 50 8B CF FF 55 60"); //0x1000F72E
    static auto FMVPtrY = pattern.get_first(1);
    static auto FMVPtrX = pattern.get_first(6);

    static auto pPresentParams = *hook::module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? 33 C0 8B D9 C1 E9 02 83 E3 03").get_first<D3DPRESENT_PARAMETERS*>(1);
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 01 00 00 00 5F 5E 5D 5B 81 C4 80 00 00 00 C2 10 00");
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 1;

            Screen.Width = *(uint32_t*)(regs.esp + 0x98); //pPresentParams->BackBufferWidth;
            Screen.Height = *(uint32_t*)(regs.esp + 0x9C); //pPresentParams->BackBufferHeight;
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

            if (Screen.Width < 640 || Screen.Height < 480)
                return;

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
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0));

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
