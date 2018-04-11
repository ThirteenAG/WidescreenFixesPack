#pragma once

#if DEBUG
#define DBGONLY(x) x
#define KEYPRESS(x) if (GetAsyncKeyState(x) & 0x8000)
#define SPDLOG_WCHAR_FILENAMES
#include "spdlog/spdlog.h"

class spd
{
public:
    inline static const std::wstring GetLogName()
    {
        HMODULE hm = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetLogName, &hm);
        std::wstring ret;
        ret.resize(MAX_PATH);
        GetModuleFileNameW(hm, &ret[0], ret.size());
        ret = ret.substr(0, ret.find_last_of('.')) + L".log";
        spdlog::set_pattern("%v");
        return ret;
    }

    /*inline*/ static const std::shared_ptr<spdlog::logger> log;
};
#else
#define DBGONLY(x)
#endif