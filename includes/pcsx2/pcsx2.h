#pragma once
#include <stdafx.h>
#include <tuple>

static constexpr uint32_t EEMainMemoryStart = 0x20000000;
static constexpr uint32_t EEMainMemoryEnd = 0x21ffffff;

struct wxSize
{
    int x, y;
};

struct wxPoint
{
    int x, y;
};

struct GSWindowOptions
{
    bool		CloseOnEsc;
    bool		DefaultToFullscreen;
    bool		AlwaysHideMouse;
    bool		DisableResizeBorders;
    bool		DisableScreenSaver;
    int         AspectRatio;
    int         FMVAspectRatioSwitch;
    int	        Zoom;
    int	        StretchY;
    int	        OffsetX;
    int	        OffsetY;
    wxSize		WindowSize;
    wxPoint		WindowPos;
    bool		IsMaximized;
    bool		IsFullscreen;
    bool		EnableVsyncWindowFlag;
    bool		IsToggleFullscreenOnDoubleClick;
};

struct PCSX2Data
{
    uint32_t* pGameCRC;
    GSWindowOptions* pGSWindowOptions;

    uint32_t GameCRC;
    uint32_t WindowWidth;
    uint32_t WindowHeight;
    uint32_t AspectRatioSwitch;
    float    AspectRatio;
    bool     Fullscreen;

    uint32_t DesktopWidth;
    uint32_t DesktopHeight;
};

class MagicNumberSt
{
public:
    void* ptr = nullptr;
    void* hi = nullptr;
    void* lo = nullptr;
    uint32_t originalValue = 0;
    void* ptrNew = nullptr;

    MagicNumberSt(void* addr, void* newVal)
    {
        ptr = addr;
        ptrNew = newVal;
        originalValue = *(uint32_t*)addr;
    }

    MagicNumberSt(void* addr, uint32_t original, void* newVal)
    {
        originalValue = original;
        ptrNew = newVal;

        constexpr uint8_t lui = 0x3C;
        constexpr uint8_t ori = 0x34;

        uint32_t bytes = injector::ReadMemory<uint32_t>(addr, true);
        uint8_t instr = (bytes >> (3 * 8)) & 0xFF;

        if (bytes == originalValue)
        {
            ptr = addr;
            return;
        }

        switch (instr)
        {
        case lui:
        {
            if (LOWORD(bytes) == HIWORD(originalValue))
            {
                hi = addr;
                for (uintptr_t i = (uint32_t)addr; i < i + 100; i++)
                {
                    bytes = injector::ReadMemory<uint32_t>(i, true);
                    instr = (bytes >> (3 * 8)) & 0xFF;

                    if ((instr == ori) && (LOWORD(bytes) == LOWORD(originalValue)))
                    {
                        lo = (void*)i;
                        break;
                    }
                }
            }
        }
        break;
        default:
            break;
        }
    }

    void WriteMemory()
    {
        if (this->ptr != nullptr)
        {
            injector::WriteMemoryRaw(this->ptr, this->ptrNew, sizeof(uint32_t), true);
        }
        else
        {
            if (this->hi != nullptr && this->lo != nullptr)
            {
                injector::WriteMemory<uint16_t>(this->hi, HIWORD(*(uint32_t*)this->ptrNew), true);
                injector::WriteMemory<uint16_t>(this->lo, LOWORD(*(uint32_t*)this->ptrNew), true);
            }
        }
    }

    bool ValuesEqual()
    {
        if (this->ptr != nullptr)
        {
            return *(uint32_t*)this->ptr == *(uint32_t*)this->ptrNew;
        }
        else
        {
            return *(uint16_t*)(this->hi) == HIWORD(*(uint32_t*)this->ptrNew) && *(uint16_t*)(this->lo) == LOWORD(*(uint32_t*)this->ptrNew);
        }
    }
};

class MagicNumberDy
{
public:
    hook::pattern pattern = {};
    size_t size = 0;
    uint32_t patternNum = 0;
    uint32_t offset = 0;
    void* newValue = nullptr;

    void* ptr = nullptr;
    uint32_t originalValue = 0;

    MagicNumberDy(hook::pattern p, size_t s, uint32_t n, uint32_t o, void* newVal) : pattern(p), size(s), patternNum(n), offset(o), newValue(newVal)
    { }

    void WriteMemory()
    {
        if (ptr)
        {
            if (*(uint32_t*)ptr != originalValue && *(uint32_t*)ptr != *(uint32_t*)newValue)
                ptr = nullptr;
        }

        if (!ptr)
        {
            if (pattern.clear().size() == 1)
            {
                ptr = pattern.get(patternNum).get<void>(offset);
                originalValue = *(uint32_t*)ptr;
            }
        }

        if (ptr)
            injector::WriteMemory(ptr, *(uint32_t*)newValue, true);
    }
};

CEXP PCSX2Data PCSX2GetData();
void WritePnach(std::wstring& filename, std::vector<MagicNumberSt>& arr, std::wstring_view gametitle, std::wstring_view comment);