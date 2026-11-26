#pragma once
#include "PS2Settings.hpp"
#include <cstdio>

// Plugins that became part of a Fusion Fix (its old Widescreen Fix, merged add-ons) patch the same game code, so the
// Fusion Fix stays off while one of them is still in PLUGINS, and says so on screen. The module that exports this
// needs PCSX2F_CFG_PEER_SETTINGS().
namespace console::ps2conflict
{
    inline uint32_t QueryPresence(const char* file)
    {
        static PCSX2FBoolPeerRequest request;
        request = {};
        request.size = sizeof(request);
        request.version = 2;
        request.operation = PCSX2F_SETTINGS_READ;
        // A read with no keys asks only whether the named module is loaded.
        if (!ps2settings::copy(request.module, file))
            return PCSX2F_SETTINGS_INVALID;
        return PCSX2F_BoolPeerRequest(&request);
    }

    // Start only after the injector confirms that every predecessor is absent.
    template<size_t Count, size_t Lines, size_t Size>
    bool Found(const char* const (&plugins)[Count], char (&osd)[Lines][Size])
    {
        static_assert(Lines >= 3);
        for (const char* file : plugins)
        {
            const auto status = QueryPresence(file);
            if (status == PCSX2F_SETTINGS_NOT_FOUND)
                continue;
            if (status == PCSX2F_SETTINGS_OK)
            {
                std::snprintf(osd[0], Size, "Old plugin in PLUGINS: %s", file);
                std::snprintf(osd[1], Size, "It is part of the Fusion Fix now and can't run next to it.");
                std::snprintf(osd[2], Size, "Delete it; the Fusion Fix stays off until then.");
            }
            else
            {
                std::snprintf(osd[0], Size, "Update PCSX2 Plugin Injector to start the Fusion Fix.");
                std::snprintf(osd[1], Size, "The installed injector couldn't check for conflicting plugins.");
            }
            return true;
        }
        return false;
    }
}
