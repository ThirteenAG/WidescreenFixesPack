#include "stdafx.h"

float GetFOV(float f, float ar)
{
    return atan2(tan(atan2(tan(f * 0.5f) / (4.0f / 3.0f), 1.0f)) * (ar), 1.0f);
}

float GetFOV2(float f, float ar)
{
    return f * (2.0f * ((180.0f / (float)M_PI) * (atan(tan(((float)M_PI / 180.0f) * ((2.0f * ((180.0f / (float)M_PI) * (atan(tan(((float)M_PI / 180.0f) * (90.0f * 0.5f)) / (4.0f / 3.0f))))) * 0.5f)) * (ar)))) * (1.0f / 90.0f));
}

float AdjustFOV(float f, float ar)
{
    return std::round((2.0f * atan(((ar) / (4.0f / 3.0f)) * tan(f / 2.0f * ((float)M_PI / 180.0f)))) * (180.0f / (float)M_PI) * 100.0f) / 100.0f;
}

void CreateThreadAutoClose(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
    CloseHandle(CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId));
}

bool IsUALPresent()
{
    ModuleList dlls;
    dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
    for (auto& e : dlls.m_moduleList)
    {
        if (GetProcAddress(std::get<HMODULE>(e), "DirectInput8Create") != NULL && GetProcAddress(std::get<HMODULE>(e), "DirectSoundCreate8") != NULL && GetProcAddress(std::get<HMODULE>(e), "InternetOpenA") != NULL)
            return true;
    }
    return false;
}

std::tuple<int32_t, int32_t> GetDesktopRes()
{
    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    int32_t DesktopResW = info.rcMonitor.right - info.rcMonitor.left;
    int32_t DesktopResH = info.rcMonitor.bottom - info.rcMonitor.top;
    return std::make_tuple(DesktopResW, DesktopResH);
}

void GetResolutionsList(std::vector<std::string>& list)
{
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
            {
                auto str = format("%dx%d", dm.dmPelsWidth, dm.dmPelsHeight);
                list.emplace_back(str);
            }
            monitorNum++;
        }
        deviceNum++;
    }

    if (list.empty())
    {
        list = { "160x120", "240x160", "320x240", "400x240", "480x320", "640x360", "640x480",
                 "768x480", "800x600", "854x480", "960x540", "960x640", "1024x576", "1024x768",
                 "1152x864", "1280x720", "1280x800", "1280x1024", "1280x1080", "1360x768", "1366x768",
                 "1400x1050", "1440x900", "1600x900", "1600x1200", "1680x1050", "1920x1080", "1920x1200",
                 "2048x1080", "2048x1152", "2048x1536", "2560x1080", "2560x1440", "2560x1600", "2560x2048",
                 "3200x1800", "3200x2048", "3200x2400", "3440x1440", "3840x1080", "3840x1600", "3840x2160",
                 "3840x2400", "4096x2160", "4096x3072", "5120x2160", "5120x2880", "5120x3200", "5120x4096",
                 "6400x4096", "6400x4800", "7680x4320", "7680x4800"
               };
    }

    std::sort(list.begin(), list.end(), [](const std::string& lhs, const std::string& rhs)
    {
        int32_t x1, y1, x2, y2;
        sscanf_s(lhs.c_str(), "%dx%d", &x1, &y1);
        sscanf_s(rhs.c_str(), "%dx%d", &x2, &y2);
        return (x1 != x2) ? (x1 < x2) : (x1 * y1 < x2 * y2);
    });
    list.erase(std::unique(std::begin(list), std::end(list)), list.end());
}

std::string format(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vector<char> v(1024);
    while (true)
    {
        va_list args2;
        va_copy(args2, args);
        int res = vsnprintf(v.data(), v.size(), fmt, args2);
        if ((res >= 0) && (res < static_cast<int>(v.size())))
        {
            va_end(args);
            va_end(args2);
            return std::string(v.data());
        }
        size_t size;
        if (res < 0)
            size = v.size() * 2;
        else
            size = static_cast<size_t>(res) + 1;
        v.clear();
        v.resize(size);
        va_end(args2);
    }
}

HICON CreateIconFromBMP(UCHAR* data)
{
    BITMAPINFOHEADER* bmih = (BITMAPINFOHEADER*)(data + sizeof(BITMAPFILEHEADER));
    HICON hIcon = 0;
    ICONINFO ii = {};
    ULONG n = bmih->biWidth * bmih->biHeight;
    PULONG lpBits = new ULONG[n];
    PULONG p = lpBits;
    if (ii.hbmColor = CreateBitmap(bmih->biWidth, bmih->biHeight, 1, bmih->biBitCount, (data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))))
    {
        if (ii.hbmMask = CreateBitmap(bmih->biWidth, bmih->biHeight, 1, 1, 0))
        {
            hIcon = CreateIconIndirect(&ii);
            DeleteObject(ii.hbmMask);
        }
        DeleteObject(ii.hbmColor);
    }
    delete[] lpBits;
    return hIcon;
}

HICON CreateIconFromResourceICO(UINT nID, int32_t cx, int32_t cy)
{
    HMODULE hm;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetDesktopRes, &hm);
    return (HICON)LoadImage(hm, MAKEINTRESOURCE(nID), IMAGE_ICON, cx, cy, LR_SHARED);
}

std::string RegistryWrapper::filter;
std::string RegistryWrapper::section;
CIniReader RegistryWrapper::RegistryReader;
std::map<std::string, std::string> RegistryWrapper::DefaultStrings;
std::set<std::string, std::less<>> RegistryWrapper::PathStrings;
std::map<std::wstring, std::function<void()>, CallbackHandler::Comparator> CallbackHandler::functions;