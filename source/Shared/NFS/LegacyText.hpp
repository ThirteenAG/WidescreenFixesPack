#pragma once
#include "Text.hpp"

namespace WFPLegacyText
{
    inline std::string Encode(const std::string& text)
    {
        std::wstring readable;
        for (auto c : WFPText::ToWide(text))
        {
            switch (c)
            {
                case L'Ł': readable += L'L'; break;
                case L'ł': readable += L'l'; break;
                case L'Œ': readable += L"OE"; break;
                case L'œ': readable += L"oe"; break;
                case L'‘': case L'’': case L'‚': readable += L'\''; break;
                case L'“': case L'”': case L'„': case L'«': case L'»': readable += L'"'; break;
                case L'–': case L'—': case L'−': case L'¬': case L'\u00AD': readable += L'-'; break;
                case L'…': readable += L"..."; break;
                case L'×': readable += L'x'; break;
                case L'±': readable += L"+/-"; break;
                case L'¼': readable += L"1/4"; break;
                case L'½': readable += L"1/2"; break;
                case L'¾': readable += L"3/4"; break;
                case L'¹': readable += L'1'; break;
                case L'²': readable += L'2'; break;
                case L'³': readable += L'3'; break;
                case L'\u00A0': readable += L' '; break;
                default: readable += c; break;
            }
        }
        auto size = WideCharToMultiByte(CP_UTF8, 0, readable.data(), int(readable.size()), nullptr, 0, nullptr, nullptr);
        std::string utf8(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, readable.data(), int(readable.size()), utf8.data(), size, nullptr, nullptr);
        // These fonts use Latin bytes, without the later games' regional slots at 0x81 and up.
        return WFPText::ToEAFont(utf8, "english");
    }
}
