#pragma once

inline std::filesystem::path MatrixNativeConfigPath()
{
    std::wstring path(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size())
        return {};
    path.resize(length);
    return std::filesystem::path(path).parent_path() / L"MatrixConfig.ini";
}

inline std::pair<int32_t, int32_t> MatrixNativeResolution()
{
    auto [width, height] = GetDesktopRes();
    if (width <= 0 || height <= 0)
    {
        width = 640;
        height = 480;
    }
    const auto path = MatrixNativeConfigPath();
    if (!path.empty())
    {
        wchar_t value[64] = {};
        const auto fallback = std::format(L"{}x{}", width, height);
        GetPrivateProfileStringW(L"RENDERING", L"RESOLUTION", fallback.c_str(), value, static_cast<DWORD>(std::size(value)), path.c_str());
        int32_t requestedWidth = 0, requestedHeight = 0;
        if (swscanf_s(value, L"%dx%d", &requestedWidth, &requestedHeight) == 2 &&
            requestedWidth > 0 && requestedHeight > 0 && requestedWidth <= 32767 && requestedHeight <= 32767)
        {
            width = requestedWidth;
            height = requestedHeight;
        }
    }
    return {width, height};
}
