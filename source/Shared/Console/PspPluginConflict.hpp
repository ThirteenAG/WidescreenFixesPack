#pragma once
#include "PSP.hpp"
#include <cstdio>
extern "C" {
#include <pspiofilemgr.h>
}

// Plugins that became part of a Fusion Fix (its old Widescreen Fix, merged add-ons) patch the same game code, so the
// Fusion Fix stays off while one of them is still in memstick/PSP/PLUGINS. PPSSPP has no on-screen message for
// plugins, so the plugin log and the emulator log say which folder to delete.
namespace console::portable::conflict
{
    // True when PLUGINS/<name>/<name>.prx exists; the caller then doesn't start.
    template<size_t Count>
    bool Found(const char* const (&names)[Count])
    {
        for (const char* name : names)
        {
            char path[160];
            std::snprintf(path, sizeof(path), "ms0:/PSP/PLUGINS/%s/%s.prx", name, name);
            SceIoStat stat{};
            if (sceIoGetstat(path, &stat) < 0)
                continue;
            char message[256];
            std::snprintf(message, sizeof(message), "%s is now part of the Fusion Fix and can't run next to it. Delete the "
                "folder memstick/PSP/PLUGINS/%s; the Fusion Fix stays off until then.\n", name, name);
            logger.Write(message);
            sceIoDevctl("kemulator:", 2, message, std::strlen(message), nullptr, 0);
            return true;
        }
        return false;
    }
}
