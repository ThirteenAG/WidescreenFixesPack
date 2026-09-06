module;

#include <stdafx.h>
#include "common.h"

export module EntryExit;

import Skeleton;

void* ms_visibleEntityList[128] = { nullptr };

class EntryExit
{
public:
    EntryExit()
    {
        // Fix for disappearing doors
        WFP::onInitEvent() += []()
        {
            // replace CEntryExitManager::ms_visibleEntityList[32]

            auto pattern = hook::pattern("83 FA ? 7D ? ? ? 0F B6 41");
            if (pattern.empty())
                return;

            injector::WriteMemory<uint8_t>(pattern.get_first(2), 127, true);

            pattern = hook::pattern("89 0C 95 ? ? ? ? 42");
            injector::WriteMemory(pattern.get_first(3), &ms_visibleEntityList[0], true);

            pattern = hook::pattern("8B 04 85 ? ? ? ? 88 48");
            injector::WriteMemory(pattern.get_first(3), &ms_visibleEntityList[0], true);
        };
    }
} EntryExit;