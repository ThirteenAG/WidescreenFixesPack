#pragma once

#include "PSP.hpp"
#include <cstdio>

namespace console::portable
{
    inline void PatchNotice(const char* message)
    {
        logger.Write(message);
        sceIoDevctl("kemulator:", 2, const_cast<char*>(message), std::strlen(message), nullptr, 0);
    }

    inline void PatchRejected(psp_hook_status status)
    {
        Unthrottle(false);
        static bool reported = false;
        if (reported)
            return;
        reported = true;
        char message[128]{};
        std::snprintf(message, sizeof(message), "Fusion Fix: patch validation failed (%u); affected patches disabled.\n", unsigned(status));
        PatchNotice(message);
    }

    inline bool BeginWithNotice()
    {
        return injector::InitializeCheckedRuntime(PatchRejected) == PSP_HOOK_OK;
    }
}
