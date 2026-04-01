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

// Try to read the PE Security Directory offset (Authenticode signature location).
// Returns the file offset where the certificate starts, or 0 if not found/not a PE.
uint64_t GetPESecurityOffset(std::ifstream& file, std::streamoff fileSize)
{
    if (fileSize < 64) return 0;

    file.seekg(0x3C);
    uint32_t peOffset = 0;
    file.read(reinterpret_cast<char*>(&peOffset), 4);
    if (!file.good() || peOffset + 24u > static_cast<uint64_t>(fileSize))
        return 0;

    file.seekg(peOffset);
    uint32_t peSig = 0;
    file.read(reinterpret_cast<char*>(&peSig), 4);
    if (peSig != 0x00004550) return 0; // "PE\0\0"

    file.seekg(peOffset + 4 + 16);
    uint16_t optHeaderSize = 0;
    file.read(reinterpret_cast<char*>(&optHeaderSize), 2);

    uint32_t optHeaderStart = peOffset + 4 + 20;

    file.seekg(optHeaderStart);
    uint16_t optMagic = 0;
    file.read(reinterpret_cast<char*>(&optMagic), 2);

    uint32_t dataDirBase;
    if (optMagic == 0x10b)       dataDirBase = optHeaderStart + 96;   // PE32
    else if (optMagic == 0x20b)  dataDirBase = optHeaderStart + 112;  // PE32+
    else return 0;

    // IMAGE_DIRECTORY_ENTRY_SECURITY is index 4 (each entry = 8 bytes)
    uint32_t secDirEntry = dataDirBase + 4 * 8;
    if (secDirEntry + 8u > optHeaderStart + optHeaderSize)
        return 0;

    file.seekg(secDirEntry);
    uint32_t secOffset = 0, secSize = 0;
    file.read(reinterpret_cast<char*>(&secOffset), 4);
    file.read(reinterpret_cast<char*>(&secSize), 4);

    if (!file.good() || secSize == 0 || secOffset < 32)
        return 0;

    return secOffset;
}

std::optional<std::vector<uint8_t>> ExtractEmbeddedPDB(const fs::path& filePath, bool forceScan)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) return {};

    auto fileSize = file.tellg();
    if (fileSize < 32) return {};

    // --- Fast path: check the very end of the file ---
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

    // --- Signed PE path: check just before the Authenticode certificate ---
    uint64_t secOffset = GetPESecurityOffset(file, fileSize);
    if (secOffset >= 32)
    {
        // Signing tools may insert alignment padding (up to 7 bytes) before
        // the certificate, so scan a small 64-byte window before it.
        constexpr std::streamoff kScanWindow = 64;
        auto scanStart = std::max(std::streamoff(0), std::streamoff(secOffset) - kScanWindow);
        auto scanLen = static_cast<size_t>(std::streamoff(secOffset) - scanStart);

        if (scanLen >= 32)
        {
            file.seekg(scanStart);
            std::vector<uint8_t> buf(scanLen);
            file.read(reinterpret_cast<char*>(buf.data()), scanLen);

            if (file.good())
            {
                for (size_t pos = scanLen - 16; pos >= 16; --pos)
                {
                    if (std::equal(kMagic.begin(), kMagic.end(), buf.begin() + pos))
                    {
                        uint64_t originalSize = *reinterpret_cast<uint64_t*>(buf.data() + pos - 16);
                        uint64_t compressedSize = *reinterpret_cast<uint64_t*>(buf.data() + pos - 8);

                        auto trailerFileOff = scanStart + std::streamoff(pos - 16);
                        if (compressedSize > 0 && compressedSize <= static_cast<uint64_t>(trailerFileOff))
                        {
                            file.seekg(trailerFileOff - std::streamoff(compressedSize));
                            std::vector<uint8_t> compressed(static_cast<size_t>(compressedSize));
                            file.read(reinterpret_cast<char*>(compressed.data()), compressedSize);

                            std::vector<uint8_t> decompressed;
                            if (DecompressData(compressed, static_cast<size_t>(originalSize), decompressed))
                                return decompressed;
                        }
                        break;
                    }
                }
            }
        }
    }

    // --- Force scan: read entire file and search backwards ---
    if (forceScan)
    {
        std::wcout << L"[EmbedPDB:INFO] Force scanning full file..." << std::endl;

        file.seekg(0);
        std::vector<uint8_t> fullData(static_cast<size_t>(fileSize));
        file.read(reinterpret_cast<char*>(fullData.data()), fileSize);

        for (size_t i = fullData.size() - 16; i >= 16; --i)
        {
            if (std::equal(kMagic.begin(), kMagic.end(), fullData.begin() + i))
            {
                uint64_t originalSize = *reinterpret_cast<uint64_t*>(fullData.data() + i - 16);
                uint64_t compressedSize = *reinterpret_cast<uint64_t*>(fullData.data() + i - 8);

                if (compressedSize == 0 || compressedSize > i - 16)
                    break;

                size_t dataStart = i - 16 - static_cast<size_t>(compressedSize);
                std::vector<uint8_t> compressed(fullData.begin() + dataStart,
                                                fullData.begin() + dataStart + compressedSize);

                std::vector<uint8_t> decompressed;
                if (DecompressData(compressed, static_cast<size_t>(originalSize), decompressed))
                    return decompressed;

                break;
            }
        }
    }

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

void ProcessFile(const fs::path& filePath, bool forceScan)
{
    if (!fs::is_regular_file(filePath))
    {
        std::wcout << L"[EmbedPDB:SKIP] Not a file: " << filePath << std::endl;
        return;
    }

    auto pdbPath = filePath.parent_path() / (filePath.stem().wstring() + L".pdb");

    bool hasPDBOnDisk = fs::exists(pdbPath);
    auto embedded = ExtractEmbeddedPDB(filePath, forceScan);
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
        std::wcout << L"Usage: EmbedPDB [-f] <file1> [file2] ...\n"
            << L"  -f   Force full file scan for embedded PDB\n"
            << L"Drag one or more files (.asi, .dll, .exe, etc.) onto this .exe" << std::endl;
        return 0;
    }

    bool forceScan = false;
    for (int i = 1; i < argc; ++i)
    {
        std::wstring arg = argv[i];
        if (arg == L"-f" || arg == L"/f")
        {
            forceScan = true;
            continue;
        }
        ProcessFile(argv[i], forceScan);
    }

    return 0;
}