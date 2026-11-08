module;

#include <stdafx.h>
#include <usercall.hpp>

export module DebugObjects;

import ComVars;

using namespace usercall;

namespace DebugTags
{
}

class DebugObjects
{
public:
    DebugObjects()
    {
        WFP::onInitEvent() += []()
        {
            // FEPkgMgr_SendMessageToPackage talks to the whole front end, so it's an ideal place to affect it globally
            onMenuScreenBaseNotify() += [](MenuScreen* menu)
            {
            };
        };
    }
} DebugObjects;