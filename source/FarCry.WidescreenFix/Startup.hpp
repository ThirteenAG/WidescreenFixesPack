#pragma once

namespace FarCryStartup
{
    inline SafetyHookInline CompileHook;

    inline size_t SkipString(std::string_view text, size_t position)
    {
        const auto quote = text[position++];
        while (position < text.size())
        {
            const auto value = text[position++];
            if (value == '\\' && position < text.size())
                ++position;
            else if (value == quote)
                return position;
        }
        return text.size();
    }

    inline void SkipTrivia(std::string_view text, size_t& position)
    {
        while (position < text.size())
        {
            if (std::isspace(static_cast<unsigned char>(text[position])))
                ++position;
            else if (text.substr(position, 2) == "--")
            {
                if (text.substr(position + 2, 2) == "[[")
                {
                    const auto end = text.find("]]", position + 4);
                    position = end == text.npos ? text.size() : end + 2;
                }
                else
                {
                    const auto end = text.find('\n', position + 2);
                    position = end == text.npos ? text.size() : end + 1;
                }
            }
            else
                break;
        }
    }

    inline std::vector<size_t> CodePositions(std::string_view text, std::string_view token)
    {
        std::vector<size_t> positions;
        size_t position = 0;
        while (position < text.size())
        {
            SkipTrivia(text, position);
            if (position >= text.size())
                break;
            const auto value = text[position];
            if (value == '\'' || value == '"')
                position = SkipString(text, position);
            else if (text.substr(position, 2) == "[[")
            {
                const auto end = text.find("]]", position + 2);
                position = end == text.npos ? text.size() : end + 2;
            }
            else
            {
                if (text.substr(position, token.size()) == token &&
                    (!position || (!std::isalnum(static_cast<unsigned char>(text[position - 1])) && text[position - 1] != '_')))
                    positions.push_back(position);
                ++position;
            }
        }
        return positions;
    }

    inline bool StartupName(const char* name)
    {
        if (!name)
            return false;
        std::string normalized(name[0] == '@' ? name + 1 : name);
        for (auto& value : normalized)
        {
            if (value == '\\')
                value = '/';
            else
                value = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
        }
        return normalized == "scripts/menuscreens/uisystem.lua";
    }

    inline std::optional<std::string> Rewrite(std::string_view source)
    {
        if (source.empty() || source.front() == '\x1B' || source.find('\0') != source.npos)
            return std::nullopt;
        constexpr std::string_view declaration = "local IntroSequence";
        constexpr std::string_view call = "VideoSequencer:Play(IntroSequence, UI.CheckOptions);";
        const auto declarations = CodePositions(source, declaration);
        const auto calls = CodePositions(source, call);
        const auto initializers = CodePositions(source, "function UI:OnInit()");
        if (declarations.size() != 1 || calls.size() != 1 || initializers.size() != 1 || initializers[0] > declarations[0])
            return std::nullopt;
        auto position = declarations[0] + declaration.size();
        const auto take = [&](char value)
        {
            SkipTrivia(source, position);
            if (position >= source.size() || source[position] != value)
                return false;
            ++position;
            return true;
        };
        if (!take('=') || !take('{'))
            return std::nullopt;
        constexpr std::string_view movies[] = { "Governmental_Message.bik", "Ubi.bik", "Crytek.bik", "Sandbox.bik", "AMD64.bik" };
        std::array<bool, std::size(movies)> found{};
        size_t count = 0;
        for (;;)
        {
            SkipTrivia(source, position);
            if (position < source.size() && source[position] == '}')
            {
                ++position;
                break;
            }
            if (!take('{'))
                return std::nullopt;
            SkipTrivia(source, position);
            if (position >= source.size() || (source[position] != '"' && source[position] != '\''))
                return std::nullopt;
            const auto begin = position + 1;
            const auto end = SkipString(source, position);
            if (end <= begin || end > source.size())
                return std::nullopt;
            const auto name = source.substr(begin, end - begin - 1);
            size_t movie = 0;
            while (movie < std::size(movies) && movies[movie] != name)
                ++movie;
            if (movie == std::size(movies) || found[movie])
                return std::nullopt;
            found[movie] = true;
            ++count;
            position = end;
            if (!take(',') || !take('1') || !take('}'))
                return std::nullopt;
            SkipTrivia(source, position);
            if (position < source.size() && source[position] == ',')
                ++position;
        }
        if (count != std::size(movies))
            return std::nullopt;
        SkipTrivia(source, position);
        if (position < source.size() && source[position] == ';')
            ++position;
        SkipTrivia(source, position);
        if (position != calls[0])
            return std::nullopt;
        std::string result(source);
        result.replace(calls[0], call.size(), "VideoSequencer.pfnEndProc = UI.CheckOptions; VideoSequencer:OnSequenceFinished();");
        return result;
    }

    inline int __cdecl Compile(void* state, const char* buffer, size_t size, const char* name)
    {
        if (StartupName(name) && buffer && size && size <= 4 * 1024 * 1024)
        {
            if (auto source = Rewrite(std::string_view(buffer, size)))
                return CompileHook.unsafe_ccall<int>(state, source->data(), source->size(), name);
            OutputDebugStringA("Far Cry: startup logo sequence differs; retaining the original script.\n");
        }
        return CompileHook.unsafe_ccall<int>(state, buffer, size, name);
    }

    inline bool Install(bool enabled, HMODULE module)
    {
        if (!enabled)
            return false;
        if (CompileHook)
            return true;
#ifndef _WIN64
        auto compile = hook::module_pattern(module, "55 8B EC FF 75 14 FF 75 10 FF 75 0C FF 75 08 E8 ? ? ? ? 83 C4 10 85 C0 75 0E 6A FF");
#else
        auto compile = hook::module_pattern(module, "48 81 EC 68 01 00 00 4D 85 C9 48 8D 05 ? ? ? ? 48 89 BC 24 88 01 00 00 49 0F 45 C1 48 8B F9 33 C9 48 85 D2");
#endif
        if (compile.size() != 1)
        {
            OutputDebugStringA("Far Cry: startup script compiler pattern unavailable.\n");
            return false;
        }
        CompileHook = safetyhook::create_inline(compile.get_first(), Compile);
        if (!CompileHook)
            OutputDebugStringA("Far Cry: startup script compiler hook unavailable.\n");
        return bool(CompileHook);
    }
}
