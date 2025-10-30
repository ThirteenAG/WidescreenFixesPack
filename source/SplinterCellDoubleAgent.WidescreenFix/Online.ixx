module;

#include "stdafx.h"
#include <d3d9.h>
#include <D3dx9.h>
#pragma comment(lib, "D3dx9.lib")
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

export module Online;

HWND g_hFocusWindow = NULL;
bool bForceWindowedMode;
bool bUsePrimaryMonitor;
bool bCenterWindow;
bool bAlwaysOnTop;
bool bDoNotNotifyOnTaskSwitch;
int nForceWindowStyle;

float fFPSLimit;
export int32_t nFrameLimitType;
export class FrameLimiter
{
public:
    enum FPSLimitMode
    {
        FPS_NONE, FPS_REALTIME, FPS_ACCURATE
    };
    FPSLimitMode mFPSLimitMode = FPS_NONE;
private:
    double TIME_Frequency = 0.0;
    double TIME_Ticks = 0.0;
    double TIME_Frametime = 0.0;
    float  fFPSLimit = 0.0f;
    bool   bFpsLimitWasUpdated = false;
public:
    void Init(FPSLimitMode mode, float fps_limit)
    {
        bFpsLimitWasUpdated = true;
        mFPSLimitMode = mode;
        fFPSLimit = fps_limit;

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        static constexpr auto TICKS_PER_FRAME = 1;
        auto TICKS_PER_SECOND = (TICKS_PER_FRAME * fFPSLimit);
        if (mFPSLimitMode == FPS_ACCURATE)
        {
            TIME_Frametime = 1000.0 / (double)fFPSLimit;
            TIME_Frequency = (double)frequency.QuadPart / 1000.0; // ticks are milliseconds
        }
        else // FPS_REALTIME
        {
            TIME_Frequency = (double)frequency.QuadPart / (double)TICKS_PER_SECOND; // ticks are 1/n frames (n = fFPSLimit)
        }
        Ticks();
    }
    DWORD Sync_RT()
    {
        if (bFpsLimitWasUpdated)
        {
            bFpsLimitWasUpdated = false;
            return 1;
        }

        DWORD lastTicks, currentTicks;
        LARGE_INTEGER counter;

        QueryPerformanceCounter(&counter);
        lastTicks = (DWORD)TIME_Ticks;
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
        currentTicks = (DWORD)TIME_Ticks;

        return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
    }
    DWORD Sync_SLP()
    {
        if (bFpsLimitWasUpdated)
        {
            bFpsLimitWasUpdated = false;
            return 1;
        }

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        double millis_current = (double)counter.QuadPart / TIME_Frequency;
        double millis_delta = millis_current - TIME_Ticks;
        if (TIME_Frametime <= millis_delta)
        {
            TIME_Ticks = millis_current;
            return 1;
        }
        else if (TIME_Frametime - millis_delta > 2.0) // > 2ms
            Sleep(1); // Sleep for ~1ms
        else
            Sleep(0); // yield thread's time-slice (does not actually sleep)

        return 0;
    }
    void Sync()
    {
        if (mFPSLimitMode == FPS_REALTIME)
            while (!Sync_RT());
        else if (mFPSLimitMode == FPS_ACCURATE)
            while (!Sync_SLP());
    }
private:
    void Ticks()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
    }
};

int nFullScreenRefreshRateInHz;
void ForceFullScreenRefreshRateInHz(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    if (!pPresentationParameters->Windowed)
    {
        std::vector<int> list;
        DISPLAY_DEVICE dd;
        dd.cb = sizeof(DISPLAY_DEVICE);
        DWORD deviceNum = 0;
        while (EnumDisplayDevices(NULL, deviceNum, &dd, 0))
        {
            DISPLAY_DEVICE newdd = { 0 };
            newdd.cb = sizeof(DISPLAY_DEVICE);
            DWORD monitorNum = 0;
            DEVMODE dm = { 0 };
            while (EnumDisplayDevices(dd.DeviceName, monitorNum, &newdd, 0))
            {
                for (auto iModeNum = 0; EnumDisplaySettings(NULL, iModeNum, &dm) != 0; iModeNum++)
                    list.emplace_back(dm.dmDisplayFrequency);
                monitorNum++;
            }
            deviceNum++;
        }

        std::sort(list.begin(), list.end());
        if (nFullScreenRefreshRateInHz > list.back() || nFullScreenRefreshRateInHz < list.front() || nFullScreenRefreshRateInHz < 0)
            pPresentationParameters->FullScreen_RefreshRateInHz = list.back();
        else
            pPresentationParameters->FullScreen_RefreshRateInHz = nFullScreenRefreshRateInHz;
    }
}

void ForceWindowed(D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode = NULL)
{
    HWND hwnd = pPresentationParameters->hDeviceWindow ? pPresentationParameters->hDeviceWindow : g_hFocusWindow;
    HMONITOR monitor = MonitorFromWindow((!bUsePrimaryMonitor && hwnd) ? hwnd : GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    int DesktopResX = info.rcMonitor.right - info.rcMonitor.left;
    int DesktopResY = info.rcMonitor.bottom - info.rcMonitor.top;

    int left = (int)info.rcMonitor.left;
    int top = (int)info.rcMonitor.top;

    if (nForceWindowStyle != 1) // not borderless fullscreen
    {
        left += (int)(((float)DesktopResX / 2.0f) - ((float)pPresentationParameters->BackBufferWidth / 2.0f));
        top += (int)(((float)DesktopResY / 2.0f) - ((float)pPresentationParameters->BackBufferHeight / 2.0f));
    }

    pPresentationParameters->Windowed = 1;

    // This must be set to default (0) on windowed mode as per D3D9 spec
    pPresentationParameters->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

    // If exists, this must match the rate in PresentationParameters
    if (pFullscreenDisplayMode != NULL)
        pFullscreenDisplayMode->RefreshRate = D3DPRESENT_RATE_DEFAULT;

    // This flag is not available on windowed mode as per D3D9 spec
    pPresentationParameters->PresentationInterval &= ~D3DPRESENT_DONOTFLIP;

    if (hwnd != NULL)
    {
        int cx, cy;
        UINT uFlags = SWP_SHOWWINDOW;
        LONG lOldStyle = GetWindowLong(hwnd, GWL_STYLE);
        LONG lOldExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        LONG lNewStyle, lNewExStyle;
        if (nForceWindowStyle == 1) // borderless fullscreen
        {
            cx = DesktopResX;
            cy = DesktopResY;
            lNewStyle = lOldStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_DLGFRAME);
            lNewStyle |= (lOldStyle & WS_CHILD) ? 0 : WS_POPUP;
            lNewExStyle = lOldExStyle & ~(WS_EX_CONTEXTHELP | WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW);
            lNewExStyle |= WS_EX_APPWINDOW;
        }
        else
        {
            cx = pPresentationParameters->BackBufferWidth;
            cy = pPresentationParameters->BackBufferHeight;
            if (!bCenterWindow)
                uFlags |= SWP_NOMOVE;

            if (nForceWindowStyle) // force windowed style
            {
                lOldExStyle &= ~(WS_EX_TOPMOST);
                lNewStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
                lNewStyle |= (nForceWindowStyle == 3) ? (WS_THICKFRAME | WS_MAXIMIZEBOX) : 0;
                lNewExStyle = (WS_EX_APPWINDOW | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE);
            }
        }
        if (nForceWindowStyle)
        {
            if (lNewStyle != lOldStyle)
            {
                SetWindowLong(hwnd, GWL_STYLE, lNewStyle);
                uFlags |= SWP_FRAMECHANGED;
            }
            if (lNewExStyle != lOldExStyle)
            {
                SetWindowLong(hwnd, GWL_EXSTYLE, lNewExStyle);
                uFlags |= SWP_FRAMECHANGED;
            }
        }
        SetWindowPos(hwnd, bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, left, top, cx, cy, uFlags);
    }
}

export void InitOnline()
{
    CIniReader iniReader("");
    bForceWindowedMode = iniReader.ReadInteger("ONLINE", "ForceWindowedMode", 0) != 0;
    fFPSLimit = static_cast<float>(iniReader.ReadInteger("ONLINE", "FPSLimit", 60));
    nFullScreenRefreshRateInHz = iniReader.ReadInteger("ONLINE", "FullScreenRefreshRateInHz", 0);
    bUsePrimaryMonitor = iniReader.ReadInteger("FORCEWINDOWED", "UsePrimaryMonitor", 0) != 0;
    bCenterWindow = iniReader.ReadInteger("FORCEWINDOWED", "CenterWindow", 1) != 0;
    bAlwaysOnTop = iniReader.ReadInteger("FORCEWINDOWED", "AlwaysOnTop", 0) != 0;
    nForceWindowStyle = iniReader.ReadInteger("FORCEWINDOWED", "ForceWindowStyle", 0);

    if (fFPSLimit)
    {
        auto mode = (iniReader.ReadInteger("ONLINE", "FPSLimitMode", 1) == 2) ? FrameLimiter::FPSLimitMode::FPS_ACCURATE : FrameLimiter::FPSLimitMode::FPS_REALTIME;
        if (mode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
            timeBeginPeriod(1);

        static FrameLimiter FpsLimiter;
        FpsLimiter.Init(mode, fFPSLimit);

        auto pattern = hook::pattern("8B F9 8B 47 30 85 C0 74 18");
        static auto fpslimiter = safetyhook::create_mid(pattern.get_first(),
            [](SafetyHookContext& ctx)
        {
            FpsLimiter.Sync();
        });
    }

    auto pattern = hook::pattern("8B 93 ? ? ? ? 50 8B 84 24 ? ? ? ? 51");
    struct CreateDeviceHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.ebx + 0x4594);

            auto pPresentationParameters = (D3DPRESENT_PARAMETERS*)(regs.ebx + 0x45E8);
            auto hFocusWindow = (HWND)regs.eax;

            g_hFocusWindow = hFocusWindow ? hFocusWindow : pPresentationParameters->hDeviceWindow;
            if (bForceWindowedMode)
                ForceWindowed(pPresentationParameters);

            if (nFullScreenRefreshRateInHz)
                ForceFullScreenRefreshRateInHz(pPresentationParameters);
        }
    }; injector::MakeInline<CreateDeviceHook>(pattern.get_first(0), pattern.get_first(6));
}
