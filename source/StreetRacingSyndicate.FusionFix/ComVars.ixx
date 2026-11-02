module;

#include <stdafx.h>

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
} Screen;


export GameRef<uint8_t> StateTop;
export GameRef<uint8_t*> WindowRenderer;
export GameRef<std::array<uint8_t, 174>> App;
export GameRef<uint8_t> SplitScreen;
export GameRef<uint8_t> BandActive;
export GameRef<int8_t> BandDirection;
export GameRef<std::array<float, 6>> BandRect, BandTarget;
export GameRef<uint32_t> BarDepth;
export GameRef<void*> MenuManager;
export GameRef<int32_t> MenuLanguage;
export GameRef<const char*> LanguageTable;
