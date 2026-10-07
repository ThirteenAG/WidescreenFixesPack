module;

#include "stdafx.h"
#include <locale>

export module Unreal;


export template<class T>
struct TArray
{
    friend struct FString;

public:
    inline TArray()
    {
        Data = nullptr;
        Count = Max = 0;
    };

    inline int Num() const
    {
        return Count;
    };

    inline T& operator[](int i)
    {
        return Data[i];
    };

    inline const T& operator[](int i) const
    {
        return Data[i];
    };

    inline bool IsValidIndex(int i) const
    {
        return i >= 0 && i < Num();
    }

public:
    T* Data;
    int Count;
    int Max;
};

export struct FString : public TArray<wchar_t>
{
    inline FString()
    {
    };

    inline bool IsValid() const
    {
        return Data != nullptr;
    }

    inline const wchar_t* c_str() const
    {
        return Data;
    }

    std::string ToString() const
    {
        if (!Data || Count <= 1)
            return {};
        const auto length = std::min<size_t>(std::wcslen(Data), Count - 1);
        std::string str(length, '\0');
        std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);
        return str;
    }
};

export enum eGameState
{
    GS_START_UP = 0,
    GS_INIT_LOGO_MPEG,
    GS_LOGO_MPEG,
    GS_INIT_INTRO_MPEG,
    GS_INTRO_MPEG,
    GS_INIT_ONCE,
    GS_INIT_FRONTEND,
    GS_FRONTEND,
    GS_INIT_PLAYING_GAME,
    GS_PLAYING_GAME,
};

