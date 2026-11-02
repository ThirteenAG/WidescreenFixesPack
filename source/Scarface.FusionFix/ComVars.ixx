module;
#include "stdafx.h"

export module ComVars;

export GameRef<void*> MainCharacter;
export GameRef<uintptr_t> profileFlowManager;
export GameRef<void*> profileCharacter;
export GameRef<void*> cameraDisplay;
export GameRef<void*> cutsceneDisplay;
export GameRef<void*> videoDisplay;
export GameRef<void*> modeDisplay;
export GameRef<void*> hudDisplay;
export GameRef<uintptr_t> controllerDescription;
export GameRef<uint8_t> vibrationDisabled;
export GameRef<uint8_t*> vibrationOutputs;
export GameRef<HWND> hWnd;
export GameRef<HWND> hWndInput;
export GameRef<HWND> hWndWindowed;
export GameRef<void*> pFlow;
export GameRef<bool> bIsInvertX;
export GameRef<bool> bIsInvertY;
export GameRef<int> nMouseLookSensitivity;
export GameRef<void*> hudManager;
export GameRef<void*> application;
export GameRef<bool> frontendDraw;

export namespace Game
{
    template<class T> T& Field(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }

}
