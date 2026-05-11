module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"

export module Hud;

import Skeleton;
import Draw;
import Sprite2d;
import Camera;
import Frontend;

export namespace CHud
{

}

class Hud
{
public:
    Hud()
    {
        WFP::onGameInitEvent() += []()
        {

        };
    }
} Hud;