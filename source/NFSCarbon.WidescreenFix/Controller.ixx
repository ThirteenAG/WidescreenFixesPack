module;

#include <stdafx.h>
#include "usercall.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module Controller;

import ComVars;

class Controller
{
public:
    Controller()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");

        };
    }
} Controller;