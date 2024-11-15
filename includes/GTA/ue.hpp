#pragma once
#include <string>
#include <locale>

template<class T>
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
        return i < Num();
    }

public:
    T* Data;
    int Count;
    int Max;
};

struct FString : public TArray<wchar_t>
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
        auto length = std::wcslen(Data);
        std::string str(length, '\0');
        std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);
        return str;
    }
};
