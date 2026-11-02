module;

#include <stdafx.h>
#include <minidx8/d3d8.h>
#include "FixSettings.hpp"
#include "LegacyHooks.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Rendering;

import ComVars;

export
{
float iniZoomFactor = 1.0f;
float fLensFlareScalar;
bool bShadowFix = true;
float fShadowScale = 1.7f;
float fDustWidth = 40.0f;

float WindowBezelSize = 1.0f / 32.0f;
float ButtonBezelOffset = 8.0f;
float AdvWindowButtonTextYOffset = 16.0f;
float AdvWindowButton2TextYOffset = 8.0f;
float AdvWindowButton2Offset = 40.0f;
float AdvWindowButton3Offset = 72.0f;
float AdvStaffRollLogoScale = 1.0f;







void UpdateViewScales()
{
    Screen.fHudZoomFactor = Screen.fAspectRatio < Screen.fConditionalAspectRatio ? Screen.fAspectRatio / Screen.fConditionalAspectRatio : 1.0f;
    Screen.fZoomFactor = Screen.fHudZoomFactor / iniZoomFactor;
    Screen.fCameraScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fZoomFactor;
    Screen.fHudScale = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fHudZoomFactor;
    Screen.HeightHUD = static_cast<int>(Screen.Height * Screen.fHudZoomFactor);
    Screen.OffsetX = static_cast<int>(Screen.Width - Screen.Width * Screen.fHudScale) / 2;
    Screen.OffsetY = static_cast<int>(Screen.Height - Screen.Height * Screen.fHudZoomFactor) / 2;
}

void updateValues(const float& newWidth, const float& newHeight)
{
    if (!std::isfinite(newWidth) || !std::isfinite(newHeight) || newWidth <= 0.0f || newHeight <= 0.0f)
        return;
    Screen.fZoomFactor = iniZoomFactor;
    //Screen resolution
    Screen.Width = static_cast<int>(newWidth);
    Screen.Height = static_cast<int>(newHeight);
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fHalfWidth = Screen.fWidth * 0.5f;
    Screen.fHalfHeight = Screen.fHeight * 0.5f;
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));

    UpdateViewScales();

    Screen.WidthFMV = Screen.Width;
    Screen.HeightFMV = Screen.Height;

    if (Screen.fAspectRatio < (4.0f / 3.0f))
    {
        Screen.HeightFMV = (int)(Screen.HeightFMV * (Screen.fAspectRatio / (4.0f / 3.0f)));
    }
    else
    {
        Screen.WidthFMV = (int)(Screen.WidthFMV / (Screen.fAspectRatio / (4.0f / 3.0f)));
    }

    fDustWidth = 40.0f * Screen.fAspectRatio;

    // write resolution vars
    ResolutionX = Screen.Width;
    ResolutionY = Screen.Height;
    ResolutionX2 = Screen.Width;
    ResolutionY2 = Screen.Height;

    OneDiv640 = 1.0f / (480.0f * Screen.fAspectRatio);

    MaestroResX = Screen.fWidth;
    MaestroResY = Screen.fHeight;

    // lens flare size
    fLensFlareScalar = Screen.fHeight / 480.0f;
    if (Screen.Width < Screen.Height)
        fLensFlareScalar = Screen.fWidth / 480.0f;

    float scalar = (Screen.fHeight / 480.0f);
    if (Screen.Width < Screen.Height)
    {
        scalar = Screen.fWidth / 480.0f;
    }
    WindowBezelSize = 1.0f / (32.0f * scalar);
    ButtonBezelOffset = 8.0f * scalar;
    AdvWindowButtonTextYOffset = 16.0f * scalar;
    AdvWindowButton2TextYOffset = 8.0f * scalar;
    AdvWindowButton2Offset = 40.0f * scalar;
    AdvWindowButton3Offset = 72.0f * scalar;
}

unsigned int GameWndProcAddr = 0;
LRESULT(WINAPI* GameWndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WSFixWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        if (WindowedModeWrapper::bScaleWindow || WindowedModeWrapper::bStretchWindow) return TRUE;

        updateValues((float)LOWORD(lParam), (float)HIWORD(lParam));
        return GameWndProc(hWnd, msg, wParam, lParam);
    }

    case WM_SIZING:
    {
        if (WindowedModeWrapper::bScaleWindow || WindowedModeWrapper::bStretchWindow) return TRUE;
        return GameWndProc(hWnd, msg, wParam, lParam);
    }

    case WM_ACTIVATE:
    {
        return TRUE;
    }
    
    }

    return GameWndProc(hWnd, msg, wParam, lParam);
}

BOOL WINAPI GetClientRectHook(HWND hWnd, LPRECT lpRect)
{
    lpRect->top = 0;
    lpRect->left = 0;
    
    lpRect->right = Screen.Width;
    lpRect->bottom = Screen.Height;
    return TRUE;
}

bool bStretchOnBootFlag = false;
uintptr_t ptrWindowSizeX = 0;
uintptr_t loc_6567BF = 0;
void InvalidateWindowSize()
{
    WindowSizeX = 640;
}



uintptr_t loc_48F41F = 0;



uintptr_t DemoRestoreExit1 = 0;


#pragma runtime_checks( "", off )
namespace AdvertiseWindowFix
{
    struct Vector2
    {
        float x;
        float y;
    };


    char* AdvWindowObj;
    void AdvWindowConstructorHook()
    {
        //printf("ADV WINDOW CONSTRUCTOR!!!\n");
        AdvWindowObj = (char*)malloc(0x848);
    }

    uintptr_t ptrAdvWindowDestructorFunc = 0;
    void __stdcall AdvWindowDestructorHook(char unk)
    {
        void* that;
        _asm mov that, ecx

        free(AdvWindowObj);

        return reinterpret_cast<void(__thiscall*)(void*, char)>(ptrAdvWindowDestructorFunc)(that, unk);
    }

    uintptr_t sub_456AA0 = 0;
    

    uintptr_t AdvertiseWindowDrawFunc3Addr = 0;
    // this is a fastcall
    // arg0 = eax
    // arg1 = ecx
    void __stdcall AdvertiseWindowDrawFunc3(Vector2* a0, Vector2* a1, uintptr_t a2)
    {
        _asm
        {
            push a2
            mov ecx, a1
            mov eax, a0
            call AdvertiseWindowDrawFunc3Addr
        }
    }

    void __stdcall AdvButtonDrawHook(uintptr_t a2)
    {
        // EAX = pos
        // ECX = size
        Vector2* inPos, * inSize;
        _asm
        {
            mov inPos, eax
            mov inSize, ecx
        }

        Vector2 newSize;
        memcpy(&newSize, inSize, sizeof(Vector2));

        float Yscalesize = Screen.fHeight / 480.0f;

        if (Screen.Width < Screen.Height)
            Yscalesize = Screen.fWidth / 480.0f;

        newSize.y *= Yscalesize;

        // since the button is shifted already to the right by the bezel size, we disregard the bezel size on the left, so it's half the bezel size!
        newSize.x -= ((32.0f * Yscalesize) / 2) - 16.0f;

        return AdvertiseWindowDrawFunc3(inPos, &newSize, a2);
    }

    uintptr_t ptrAdvWindowDrawFunc = 0;
    void __stdcall AdvWindowDrawHook()
    {
        uintptr_t that;
        _asm mov that, ecx

        // make a copy of the object
        memcpy(AdvWindowObj, (void*)that, 0x848);

        float Xscale = Screen.Width43 / 640.0f;
        float Yscale = Screen.fHeight / 480.0f;
        float Yscalesize = Yscale;

        if (Screen.Width < Screen.Height)
        {
            Xscale = Screen.fWidth / 640.0f;
            Yscalesize = Screen.fWidth / 480.0f;
        }

        *(float*)(&AdvWindowObj[0x48]) *= Yscalesize; // Xsize
        *(float*)(&AdvWindowObj[0x4C]) *= Yscalesize; // Ysize
        *(float*)(&AdvWindowObj[0x3C]) *= Xscale; // Xpos
        if (Screen.fAspectRatio != (4.0f / 3.0f))
        {
            *(float*)(&AdvWindowObj[0x3C]) += static_cast<float>((Screen.Width - Screen.Width43) / 2.0f);
            if (Screen.fAspectRatio < (4.0f / 3.0f))
                if (*(float*)(&AdvWindowObj[0x3C]) < 0) *(float*)(&AdvWindowObj[0x3C]) = 0;
        }
        *(float*)(&AdvWindowObj[0x40]) *= Yscale; // Ypos

        reinterpret_cast<void(__thiscall*)(char*)>(ptrAdvWindowDrawFunc)(AdvWindowObj);
    }
}

namespace AdvStaffRollFix
{
    struct typeStaffrollTextObj
    {
        uint32_t unk1;
        uint32_t unk2;
        float posX;
        float posY;
        float size;
        uint32_t unk4;
    };

    char* StaffrollObjCopy;
    typeStaffrollTextObj* scaledStaffRollObjs;
    void __stdcall AdvStaffrollConstructorHook(char* StaffrollObj)
    {
        StaffrollObjCopy = (char*)malloc(0xA4);

        int objCount = *(uint32_t*)(&StaffrollObj[0x88]);
        uint32_t TextObjSize = sizeof(typeStaffrollTextObj) * objCount;

        typeStaffrollTextObj* TextObjs = *(typeStaffrollTextObj**)(&StaffrollObj[0x84]);
        scaledStaffRollObjs = (typeStaffrollTextObj*)malloc(TextObjSize);
    }

    uintptr_t sub_454F10 = 0;
    

    uintptr_t ptrAdvStaffrollDestructorFunc = 0;
    void __stdcall AdvStaffrollDestructorHook(char unk)
    {
        void* that;
        _asm mov that, ecx

        free(StaffrollObjCopy);
        free(scaledStaffRollObjs);

        return reinterpret_cast<void(__thiscall*)(void*, char)>(ptrAdvStaffrollDestructorFunc)(that, unk);
    }

    uintptr_t ptrAdvStaffrollDrawFunc = 0;
    void __stdcall AdvStaffrollDrawHook()
    {
        char* that;
        _asm mov that, ecx

        // make a copy of the object
        memcpy(StaffrollObjCopy, that, 0xA4);

        uint32_t objtype = *(uint32_t*)(&StaffrollObjCopy[0x28]);

        if (objtype == 5)
        {
            int objCount = *(uint32_t*)(&StaffrollObjCopy[0x88]);
            uint32_t TextObjSize = sizeof(typeStaffrollTextObj) * objCount;

            typeStaffrollTextObj* TextObjs = *(typeStaffrollTextObj**)(&StaffrollObjCopy[0x84]);
            memcpy(scaledStaffRollObjs, TextObjs, TextObjSize);

            for (int i = 0; i < objCount; i++)
            {
                float scalar = Screen.fHeight / 480.0f;
                float Xscale = Screen.Width / 640.0f;

                if (Screen.Width < Screen.Height)
                {
                    scalar = Screen.fWidth / 480.0f;
                    Xscale = Screen.fWidth / 640.0f;
                }

                scaledStaffRollObjs[i].posX *= Xscale;
                scaledStaffRollObjs[i].posY *= scalar;
            }

            *(typeStaffrollTextObj**)(&StaffrollObjCopy[0x84]) = scaledStaffRollObjs;
        }

        reinterpret_cast<void(__thiscall*)(char*)>(ptrAdvStaffrollDrawFunc)(StaffrollObjCopy);
    }

    uintptr_t AdvStaffrollLogoFuncAddr = 0;
    // this is a fastcall
    // arg0 = esi
    void __stdcall AdvStaffrollLogoFunc(void* obj, float sizeX, float sizeY)
    {
        _asm
        {
            push sizeY
            push sizeX
            mov esi, obj
            call AdvStaffrollLogoFuncAddr
        }
    }

    void __stdcall AdvStaffrollLogoHook(float sizeX, float sizeY)
    {
        typeStaffrollTextObj* that;
        _asm mov that, esi

        float scalar = Screen.fHeight / 480.0f;
        float posscalar = (4.0f / 3.0f) / Screen.fAspectRatio;

        that->posX *= posscalar;

        if (Screen.Width < Screen.Height)
        {
            scalar = Screen.fWidth / 480.0f;
            that->posX = 0;
        }				

        return AdvStaffrollLogoFunc(that, sizeX * scalar, sizeY * scalar);
    }
}

uintptr_t TextDrawFunc2Addr = 0;
// this is a fastcall
// arg0 = eax
void __stdcall TextDrawFunc2(uintptr_t a0, uintptr_t a1, float posX, float posY, float sizeX, float sizeY, uintptr_t a3)
{
    _asm
    {
        push a3
        push sizeY
        push sizeX
        push posY
        push posX
        push a1
        mov eax, a0
        call TextDrawFunc2Addr
    }
}

void __stdcall TextDrawFunc2Hook(uintptr_t a1, float posX, float posY, float sizeX, float sizeY, uintptr_t a3)
{
    uintptr_t a0;
    _asm mov a0, eax

    float scalar = (Screen.fHeight / 480.0f);
    //float posXscalar = (Screen.fAspectRatio / (4.0f / 3.0f));

    if (Screen.fAspectRatio < (4.0f / 3.0f))
        scalar = (Screen.fWidth / 480.0f);

    float newSizeX = sizeX * scalar;
    float newSizeY = sizeY * scalar;
    //float newPosX = posX * TestFloat1;

    //printf("PosX: %.2f\tNewPosX: %.2f\n", posX, newPosX);

    return TextDrawFunc2(a0, a1, posX, posY, newSizeX, newSizeY, a3);
}


#pragma runtime_checks( "", restore )

ATOM WINAPI RegisterClassHook(WNDCLASSEXA* wcex) 
{
    // TODO -- integrate icon into a resource
    wcex->hIcon = (HICON)LoadImage(NULL, L"icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
    return RegisterClassExA(wcex);
}

BOOL WINAPI UpdateWindowHook(HWND hWnd)
{
    // fix the window to open at the center of the screen...
    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX info = { sizeof(MONITORINFOEX) };
    GetMonitorInfo(monitor, &info);
    DEVMODE devmode = {};
    devmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
    DWORD DesktopX = devmode.dmPelsWidth;
    DWORD DesktopY = devmode.dmPelsHeight;

    RECT wndRect = { 0 };
    GetWindowRect(hWnd, &wndRect);

    int Width = wndRect.right - wndRect.left;
    int Height = wndRect.bottom - wndRect.top;

    int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)Width / 2.0f));
    int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)Height / 2.0f));

    SetWindowPos(hWnd, 0, WindowPosX, WindowPosY, 0, 0, SWP_NOSIZE | SWP_FRAMECHANGED);

    return UpdateWindow(hWnd);
}



// The notice comes as a message box like the game's own quit question, once the main menu runs


enum SystemMode
{
    PalSelect,
    MainMenu,
    InGame,
    EasyMenu,
    Credits,
    EasyMenuMovie,
    MaxSysMode
};


}
