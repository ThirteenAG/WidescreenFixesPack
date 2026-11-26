#pragma once

#include <cwchar>
#include <format>
#include <string>
#include <string_view>

namespace SCPadAxis
{
    inline std::wstring ScaleCommands(std::wstring_view commands, float factor)
    {
        std::wstring result;
        for (size_t start = 0; start < commands.size();)
        {
            const auto delimiter = commands.find_first_of(L"|\r\n", start);
            const auto end = delimiter == std::wstring_view::npos ? commands.size() : delimiter;
            const auto command = commands.substr(start, end - start);
            const auto first = command.find_first_not_of(L" \t");
            if (first != std::wstring_view::npos && command.size() > first + 4 &&
                !_wcsnicmp(command.data() + first, L"Axis", 4) && (command[first + 4] == L' ' || command[first + 4] == L'\t'))
            {
                bool hasSpeed = false;
                for (size_t option = first + 5; option + 6 <= command.size(); ++option)
                {
                    if ((command[option - 1] == L' ' || command[option - 1] == L'\t') &&
                        !_wcsnicmp(command.data() + option, L"Speed=", 6))
                    {
                        auto stop = command.find_first_of(L" \t", option + 6);
                        if (stop == std::wstring_view::npos)
                            stop = command.size();
                        const std::wstring value(command.substr(option + 6, stop - option - 6));
                        result.append(command.substr(0, option + 6));
                        result += std::format(L"{:.6g}", std::wcstof(value.c_str(), nullptr) * factor);
                        result.append(command.substr(stop));
                        hasSpeed = true;
                        break;
                    }
                }
                if (!hasSpeed)
                {
                    result.append(command);
                    result += std::format(L" Speed={:.6g}", factor);
                }
            }
            else
                result.append(command);

            if (end == commands.size())
                break;
            result += commands[end];
            start = end + 1;
        }
        return result;
    }
}
