module;

#include <stdafx.h>
#include <algorithm>
#include <string>
#include <string_view>

export module ControllerArtwork;

export namespace ControllerArtwork
{
    inline int Index(std::string_view path)
    {
        std::string name(path);
        for (auto& character : name)
        {
            if (character == '\\')
                character = '/';
            else if (character >= 'A' && character <= 'Z')
                character += 'a' - 'A';
        }
        const auto slash = name.find_last_of('/');
        if (slash == std::string::npos)
            return 0;
        const std::string_view directory(name.data(), slash);
        const bool prompts = directory == "flash/textures/sc6iconspc" || directory.ends_with("/flash/textures/sc6iconspc");
        const bool controls = directory == "flash/textures/menu_controls" || directory.ends_with("/flash/textures/menu_controls");
        if (!prompts && !controls)
            return 0;
        auto file = std::string_view(name).substr(slash + 1);
        if (!file.starts_with("icon_ik_joy") || !file.ends_with(".dds"))
            return 0;
        file.remove_prefix(11);
        file.remove_suffix(4);
        if (file.empty() || file.size() > 2 || file.front() == '0')
            return 0;
        int index = 0;
        for (const auto character : file)
        {
            if (character < '0' || character > '9')
                return 0;
            index = index * 10 + character - '0';
        }
        if (index < 1 || index > 16 || (controls && index != 1 && index != 5 && index != 6))
            return 0;
        return index;
    }
}
