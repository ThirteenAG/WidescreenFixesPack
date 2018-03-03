#include "stdafx.h"

float GetFOV(float f, float ar)
{
    return atan2(tan(atan2(tan(f * 0.5f) / (4.0f / 3.0f), 1.0f)) * (ar), 1.0f);
}

void CreateThreadAutoClose(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
    CloseHandle(CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId));
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
    DEVMODE dm = { 0 };
    for (auto iModeNum = 0; EnumDisplaySettings(NULL, iModeNum, &dm) != 0; iModeNum++)
    {
        auto str = format("%dx%d", dm.dmPelsWidth, dm.dmPelsHeight);
        list.push_back(str);
    }
    list.erase(std::unique(std::begin(list), std::end(list)), list.end());
    std::sort(list.begin(), list.end(), [](const std::string& lhs, const std::string& rhs) {
        int32_t x1, y1, x2, y2;
        sscanf(lhs.c_str(), "%dx%d", &x1, &y1);
        sscanf(rhs.c_str(), "%dx%d", &x2, &y2);
        return (x1 != x2) ? (x1 < x2) : (x1 * y1 < x2 * y2);
    });
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