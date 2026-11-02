#pragma once

#include <array>
#include <cctype>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace SplinterCell
{
    class PromptCache
    {
        uint64_t revision = 0;
        std::array<std::string, 16> bindings;
        std::array<std::wstring, 17> buttons;
        std::map<std::string, int, std::less<>> commands;
        std::map<std::wstring, std::wstring, std::less<>> prompts;

    public:
        bool Current(uint64_t current) const
        {
            return revision == current;
        }

        template<class ReadBinding, class NameButton>
        void Refresh(uint64_t current, ReadBinding readBinding, NameButton nameButton)
        {
            if (revision == current)
                return;
            commands.clear();
            prompts.clear();
            for (int i = 1; i <= 16; ++i)
            {
                bindings[i - 1] = readBinding(i);
                buttons[i] = nameButton(i);
            }
            revision = current;
        }

        int BoundButton(std::string_view command)
        {
            if (const auto found = commands.find(command); found != commands.end())
                return found->second;
            int button = 0;
            for (size_t i = 0; i < bindings.size() && !button; ++i)
            {
                const auto& binding = bindings[i];
                for (size_t at = binding.find(command); at != std::string::npos; at = binding.find(command, at + 1))
                {
                    const size_t end = at + command.size();
                    if ((at == 0 || !isalnum(static_cast<unsigned char>(binding[at - 1]))) &&
                        (end >= binding.size() || !isalnum(static_cast<unsigned char>(binding[end]))))
                    {
                        button = int(i + 1);
                        break;
                    }
                }
            }
            commands.emplace(command, button);
            return button;
        }

        const std::wstring& ButtonName(int button) const
        {
            static const std::wstring unknown = L"?";
            return button >= 1 && button <= 16 ? buttons[button] : unknown;
        }

        template<class Transform>
        std::wstring Prompt(std::wstring_view original, Transform transform)
        {
            if (const auto found = prompts.find(original); found != prompts.end())
                return found->second;
            std::wstring result(original);
            if (!transform(result))
                result.clear();
            // Dynamic HUD counters must not grow the cache indefinitely.
            if (prompts.size() >= 512)
                prompts.clear();
            prompts.emplace(original, result);
            return result;
        }
    };
}
