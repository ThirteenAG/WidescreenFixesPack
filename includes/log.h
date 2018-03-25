#pragma once

#if DEBUG
#define DBGONLY(x) x
#define KEYPRESS(x) if (GetAsyncKeyState(x) & 0x8000)
#include "spdlog/spdlog.h"

class spd
{
private:
    inline static const std::string GetLogName()
    {
        HMODULE hm = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetLogName, &hm);
        std::string ret;
        ret.resize(MAX_PATH);
        GetModuleFileNameA(hm, &ret[0], ret.size());
        ret = ret.substr(0, ret.find_last_of('.')) + ".log";
        spdlog::set_pattern("%v");
        return ret;
    }

public:
    inline static const std::shared_ptr<spdlog::logger> log = spdlog::basic_logger_mt("basic_logger", GetLogName(), true);
};
#else
#define DBGONLY(x)
#endif