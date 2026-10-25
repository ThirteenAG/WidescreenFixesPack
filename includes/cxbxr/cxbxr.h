#pragma once
#include <Windows.h>
#include <filesystem>
#include <tlhelp32.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

void GetAllWindowsFromProcessID(DWORD dwProcessID, std::vector<HWND>& vhWnds)
{
    HWND hCurWnd = nullptr;
    do
    {
        hCurWnd = FindWindowEx(nullptr, hCurWnd, nullptr, nullptr);
        DWORD checkProcessID = 0;
        GetWindowThreadProcessId(hCurWnd, &checkProcessID);
        if (checkProcessID == dwProcessID)
        {
            vhWnds.push_back(hCurWnd);
        }
    }
    while (hCurWnd != nullptr);
}

DWORD getParentPID(DWORD pid)
{
    PROCESSENTRY32 pe = { 0 };
    DWORD ppid = 0;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(h, &pe))
    {
        do
        {
            if (pe.th32ProcessID == pid)
            {
                ppid = pe.th32ParentProcessID;
                break;
            }
        }
        while (Process32Next(h, &pe));
    }
    CloseHandle(h);
    return (ppid);
}

std::filesystem::path getProcessName(DWORD pid)
{
    wchar_t fname[MAX_PATH] = { 0 };
    auto e = 0;
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (h)
    {
        if (GetModuleFileNameEx(h, NULL, fname, MAX_PATH) == 0)
            e = GetLastError();
        CloseHandle(h);
    }
    else
    {
        e = GetLastError();
    }
    if (!e)
        return std::filesystem::path(fname);
    else
        return "";
}

std::tuple<uint32_t, uint32_t> getWindowDimensions(DWORD pid)
{
    std::vector<HWND> windows;
    GetAllWindowsFromProcessID(pid, windows);
    uint32_t width = 0, height = 0;
    uint64_t largest = 0;
    for (auto window : windows)
    {
        RECT client{};
        if (!window || !IsWindowVisible(window) || !GetClientRect(window, &client) || client.right <= 0 || client.bottom <= 0)
            continue;
        const auto area = uint64_t(client.right) * client.bottom;
        if (area > largest)
        {
            largest = area;
            width = client.right;
            height = client.bottom;
        }
    }
    return std::make_tuple(width, height);
}
