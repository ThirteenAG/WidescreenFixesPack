module;

#include <stdafx.h>

export module ComVars;

export struct INI
{
    int WindowMode;
    bool SkipIntro;
} IniFile;

export uint32_t gc_16C900;
export uint32_t gc_16DD00;
export GameRef<void*> interfaceManager;
export GameRef<void*> provider;
export GameRef<void*> overlayFirst;
export GameRef<void*> overlaySecond;
export GameRef<void*> connection;
export GameRef<void*> gameState;
