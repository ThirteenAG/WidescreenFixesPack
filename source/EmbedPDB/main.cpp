#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <string>
#include <array>
#include <algorithm>
#include <optional>
#include "miniz.h"

namespace fs = std::filesystem;

constexpr std::array<uint8_t, 16> kMagic = { 'P','D','B','_','E','M','B','E','D','_','V','1', 0,0,0,0 };

bool CompressData(const std::vector<uint8_t>& inData, std::vector<uint8_t>& outCompressed)
{
    mz_ulong compSize = mz_compressBound(static_cast<mz_ulong>(inData.size()));
    outCompressed.resize(compSize);

    int status = mz_compress2(outCompressed.data(), &compSize,
                              inData.data(), mz_ulong(inData.size()), MZ_BEST_COMPRESSION);

    if (status != MZ_OK) return false;
    outCompressed.resize(compSize);
    return true;
}

bool DecompressData(const std::vector<uint8_t>& compressed, size_t originalSize, std::vector<uint8_t>& outDecompressed)
{
    outDecompressed.resize(originalSize);
    mz_ulong destLen = static_cast<mz_ulong>(originalSize);

    int status = mz_uncompress(outDecompressed.data(), &destLen,
                               compressed.data(), mz_ulong(compressed.size()));

    return (status == MZ_OK) && (destLen == originalSize);
}

std::optional<std::vector<uint8_t>> ExtractEmbeddedPDB(const fs::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) return {};

    auto fileSize = file.tellg();
    if (fileSize < 32) return {};

    file.seekg(fileSize - std::streamoff(32));
    std::array<uint8_t, 32> tail{};
    file.read(reinterpret_cast<char*>(tail.data()), 32);

    if (std::equal(kMagic.begin(), kMagic.end(), tail.begin() + 16))
    {
        uint64_t originalSize = *reinterpret_cast<uint64_t*>(tail.data());
        uint64_t compressedSize = *reinterpret_cast<uint64_t*>(tail.data() + 8);

        if (compressedSize == 0 || compressedSize > static_cast<uint64_t>(fileSize) - 32)
            return {};

        file.seekg(fileSize - std::streamoff(32) - std::streamoff(compressedSize));
        std::vector<uint8_t> compressed(compressedSize);
        file.read(reinterpret_cast<char*>(compressed.data()), compressedSize);

        std::vector<uint8_t> decompressed;
        if (DecompressData(compressed, originalSize, decompressed))
            return decompressed;
    }

    //std::wcout << L"[EmbedPDB:INFO] Magic not at end - scanning full file..." << std::endl;
    //
    //file.seekg(0);
    //std::vector<uint8_t> fullData(static_cast<size_t>(fileSize));
    //file.read(reinterpret_cast<char*>(fullData.data()), fileSize);
    //
    //for (size_t i = fullData.size() - 16; i != static_cast<size_t>(-1); --i)
    //{
    //    if (std::equal(kMagic.begin(), kMagic.end(), fullData.begin() + i))
    //    {
    //        size_t pos = i + 16;
    //        if (pos + 16 > fullData.size()) break;
    //
    //        uint64_t originalSize = *reinterpret_cast<uint64_t*>(fullData.data() + pos);
    //        uint64_t compressedSize = *reinterpret_cast<uint64_t*>(fullData.data() + pos + 8);
    //
    //        if (compressedSize == 0 || pos + 16 + compressedSize > fullData.size())
    //            break;
    //
    //        std::vector<uint8_t> compressed(fullData.begin() + pos + 16,
    //                                        fullData.begin() + pos + 16 + compressedSize);
    //
    //        std::vector<uint8_t> decompressed;
    //        if (DecompressData(compressed, originalSize, decompressed))
    //            return decompressed;
    //
    //        break;
    //    }
    //}

    return {};
}

bool EmbedPDB(const fs::path& targetPath, const fs::path& pdbPath)
{
    std::ifstream pdbFile(pdbPath, std::ios::binary | std::ios::ate);
    if (!pdbFile) return false;

    auto originalSize = pdbFile.tellg();
    pdbFile.seekg(0);
    std::vector<uint8_t> pdbData(static_cast<size_t>(originalSize));
    pdbFile.read(reinterpret_cast<char*>(pdbData.data()), originalSize);

    std::vector<uint8_t> compressed;
    if (!CompressData(pdbData, compressed)) return false;

    std::ofstream targetFile(targetPath, std::ios::binary | std::ios::app);
    if (!targetFile) return false;

    targetFile.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    uint64_t orig = originalSize;
    uint64_t comp = compressed.size();
    targetFile.write(reinterpret_cast<const char*>(&orig), 8);
    targetFile.write(reinterpret_cast<const char*>(&comp), 8);
    targetFile.write(reinterpret_cast<const char*>(kMagic.data()), 16);

    double ratio = (static_cast<double>(originalSize) / compressed.size()) * 100.0;
    std::wcout << L"[EmbedPDB:SUCCESS] Embedded into: " << targetPath << L"\n"
        << L"   Original: " << originalSize / 1024 << L" KB\n"
        << L"   Compressed: " << compressed.size() / 1024 << L" KB ("
        << ratio << L"% of original)" << std::endl;

    return true;
}

void ProcessFile(const fs::path& filePath)
{
    if (!fs::is_regular_file(filePath))
    {
        std::wcout << L"[EmbedPDB:SKIP] Not a file: " << filePath << std::endl;
        return;
    }

    auto pdbPath = filePath.parent_path() / (filePath.stem().wstring() + L".pdb");

    bool hasPDBOnDisk = fs::exists(pdbPath);
    auto embedded = ExtractEmbeddedPDB(filePath);
    bool hasEmbedded = embedded.has_value();

    if (hasPDBOnDisk && hasEmbedded)
    {
        std::wcout << L"[EmbedPDB:INFO] Already up to date: " << filePath << std::endl;
    }
    else if (hasPDBOnDisk)
    {
        EmbedPDB(filePath, pdbPath);
    }
    else if (hasEmbedded)
    {
        std::ofstream out(pdbPath, std::ios::binary);
        if (out)
        {
            const auto& d = *embedded;
            out.write(reinterpret_cast<const char*>(d.data()), d.size());
            std::wcout << L"[EmbedPDB:SUCCESS] Extracted: " << pdbPath << std::endl;
        }
        else
            std::wcout << L"[EmbedPDB:ERROR] Failed to write: " << pdbPath << std::endl;
    }
    else
    {
        std::wcout << L"[EmbedPDB:INFO] No PDB found for: " << filePath << std::endl;
    }
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc == 1)
    {
        std::wcout << L"Drag one or more files (.asi, .dll, .exe, etc.) onto this .exe" << std::endl;
        return 0;
    }

    for (int i = 1; i < argc; ++i)
        ProcessFile(argv[i]);

    return 0;
}