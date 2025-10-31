module;

#include <stdafx.h>

export module WidescreenHUD;

import ComVars;

export struct WidescreenHudOffset
{
    int32_t _int;
    float _float;
} WidescreenHudOffset;

export struct TextOffset
{
    struct
    {
        int32_t v1v1 = 435;
        int32_t v1v2 = 436;
        int32_t v1v3 = 437;
        int32_t v2v1 = 3;
        int32_t v2v2 = 21;
        int32_t v3v1 = 313;
        int32_t v3v2 = 329;
        int32_t v3v3 = 345;
        int32_t v3v4 = 361;
        int32_t v3v5 = 377;
        int32_t v3v6 = 393;
        int32_t v3v7 = 416;
    } topCorner;
    struct
    {
        int32_t v1v1 = 489;
        int32_t v1v2 = 598;
        int32_t v2v1 = 1;
        int32_t v2v2 = 20;
        int32_t v3v1 = 23;
        int32_t v3v2 = 39;
        int32_t v3v3 = 93;
    } bottomCorner;
    struct
    {
        int32_t v1 = 598;
        int32_t v2 = 3;
        int32_t v3 = 93;
    } icons;
    struct
    {
        int32_t v1v1 = 465;
        int32_t v1v2 = 615;
        int32_t v2v1 = 3;
        int32_t v2v2 = 75;
        int32_t v3v1 = 128;
        int32_t v3v2 = 215;
    } objPopup;
} sTextOffset;
