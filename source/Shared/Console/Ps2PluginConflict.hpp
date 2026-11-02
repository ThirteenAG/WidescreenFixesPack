#pragma once
#include "PS2Settings.hpp"
#include <cstdio>

// Plugins that became part of a Fusion Fix (its old Widescreen Fix, merged add-ons) patch the same game code, so the
// Fusion Fix stays off while one of them is still in PLUGINS, and says so on screen. The module that exports this
// needs PCSX2F_CFG_PEER_SETTINGS().
namespace console::ps2conflict
{
    // A key the old plugin always writes to its CFG.
    struct OldPlugin
    {
        const char* file;
        const char* section;
        const char* key;
    };

    // The injector's peer service reports NOT_FOUND for a module that isn't loaded. A loaded one answers with its
    // value, or INVALID when it has no CFG (old INI plugins). One that never ran yet has no CFG file and isn't seen
    // until its first start.
    inline bool Loaded(const OldPlugin& plugin)
    {
        static PCSX2FBoolPeerRequest request;
        request = {};
        request.size = sizeof(request);
        request.version = 2;
        request.operation = PCSX2F_SETTINGS_READ;
        request.count = 1;
        if (!ps2settings::copy(request.module, plugin.file) || !ps2settings::copy(request.section, plugin.section) ||
            !ps2settings::copy(request.key, plugin.key))
            return false;
        const auto status = PCSX2F_BoolPeerRequest(&request);
        return status != PCSX2F_SETTINGS_NOT_FOUND && status != PCSX2F_SETTINGS_UNSUPPORTED;
    }

    // True when one of the old plugins is loaded; the caller then doesn't start. The message takes the first three
    // OSD lines.
    template<size_t Count, size_t Lines, size_t Size>
    bool Found(const OldPlugin (&plugins)[Count], char (&osd)[Lines][Size])
    {
        static_assert(Lines >= 3);
        for (const auto& plugin : plugins)
        {
            if (!Loaded(plugin))
                continue;
            std::snprintf(osd[0], Size, "Old plugin in PLUGINS: %s", plugin.file);
            std::snprintf(osd[1], Size, "It is part of the Fusion Fix now and can't run next to it.");
            std::snprintf(osd[2], Size, "Delete it; the Fusion Fix stays off until then.");
            return true;
        }
        return false;
    }
}
