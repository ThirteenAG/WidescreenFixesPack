module;

#include <stdafx.h>
#include <d3d9.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

export module OptionWord;

// The entries of the game's Option list are pictures in its option01 texture: one 500 x 40 picture per word and
// language, with the glow of the selected entry (left half) and without it (right half). A word the game doesn't have
// is put together from the letters of those pictures, taken from the textures of every language in the title archive,
// and written into an unused part of the texture, so the game draws it, selects it and animates it like its own.
namespace REWordParts
{
    constexpr int Width = 500, Height = 40, TextureSize = 1024;
    // The first word row of the texture, and where the composed word goes (a part no language texture uses)
    constexpr int FirstRow = 340, Rows = 9, TargetRow = 940;
    // How far a letter's glow reaches around its strokes
    constexpr int Spread = 18;
    // The rows at the top of a word's cell that belong to the cell above
    constexpr int TopRows = 3;

    struct Pixel
    {
        uint8_t r = 0, g = 0, b = 0, a = 0;
    };

    // zlib streams of the game's archives
    class Inflater
    {
    public:
        Inflater(const uint8_t* data, size_t size) : data(data), size(size) {}

        bool Run(std::vector<uint8_t>& out)
        {
            if (size < 2 || (data[0] & 15) != 8) return false;
            position = 2;
            for (bool last = false; !last && !failed;)
            {
                last = Bits(1) != 0;
                const auto type = Bits(2);
                if (type == 0) Stored(out);
                else if (type == 1) Fixed(out);
                else if (type == 2) Dynamic(out);
                else return false;
            }
            return !failed;
        }

    private:
        struct Huffman
        {
            std::array<uint16_t, 16> count{};
            std::array<uint16_t, 320> symbol{};
        };

        const uint8_t* data;
        size_t size, position = 0;
        uint32_t buffer = 0, available = 0;
        bool failed = false;

        uint32_t Bits(int n)
        {
            uint32_t value = buffer;
            while (available < static_cast<uint32_t>(n))
            {
                if (position >= size) { failed = true; return 0; }
                value |= static_cast<uint32_t>(data[position++]) << available;
                available += 8;
            }
            buffer = value >> n;
            available -= n;
            return value & ((1u << n) - 1);
        }

        static void Build(Huffman& table, const uint8_t* lengths, int n)
        {
            table.count.fill(0);
            for (int i = 0; i < n; ++i) ++table.count[lengths[i]];
            table.count[0] = 0;
            std::array<uint16_t, 16> offsets{};
            for (int length = 1; length < 15; ++length) offsets[length + 1] = offsets[length] + table.count[length];
            for (int i = 0; i < n; ++i)
                if (lengths[i]) table.symbol[offsets[lengths[i]]++] = static_cast<uint16_t>(i);
        }

        int Decode(const Huffman& table)
        {
            int code = 0, first = 0, index = 0;
            for (int length = 1; length < 16; ++length)
            {
                code |= static_cast<int>(Bits(1));
                const int count = table.count[length];
                if (code - count < first) return table.symbol[index + (code - first)];
                index += count;
                first = (first + count) << 1;
                code <<= 1;
            }
            failed = true;
            return 0;
        }

        void Stored(std::vector<uint8_t>& out)
        {
            buffer = available = 0;
            if (position + 4 > size) { failed = true; return; }
            const size_t length = data[position] | (data[position + 1] << 8);
            position += 4;
            if (position + length > size) { failed = true; return; }
            out.insert(out.end(), data + position, data + position + length);
            position += length;
        }

        void Codes(std::vector<uint8_t>& out, const Huffman& literals, const Huffman& distances)
        {
            static constexpr uint16_t lengthBase[29]{ 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
                67, 83, 99, 115, 131, 163, 195, 227, 258 };
            static constexpr uint8_t lengthExtra[29]{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4,
                5, 5, 5, 5, 0 };
            static constexpr uint16_t distanceBase[30]{ 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
                513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
            static constexpr uint8_t distanceExtra[30]{ 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
                10, 11, 11, 12, 12, 13, 13 };
            while (!failed)
            {
                int symbol = Decode(literals);
                if (symbol < 256) { out.push_back(static_cast<uint8_t>(symbol)); continue; }
                if (symbol == 256) return;
                symbol -= 257;
                if (symbol >= 29) { failed = true; return; }
                const size_t length = lengthBase[symbol] + Bits(lengthExtra[symbol]);
                const int code = Decode(distances);
                if (code >= 30) { failed = true; return; }
                const size_t distance = distanceBase[code] + Bits(distanceExtra[code]);
                if (failed || distance > out.size()) { failed = true; return; }
                for (size_t i = 0; i < length; ++i) out.push_back(out[out.size() - distance]);
            }
        }

        void Fixed(std::vector<uint8_t>& out)
        {
            static const auto tables = []()
            {
                std::array<uint8_t, 288> lengths{};
                for (int i = 0; i < 288; ++i) lengths[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
                std::pair<Huffman, Huffman> result;
                Build(result.first, lengths.data(), 288);
                std::array<uint8_t, 30> distances{};
                distances.fill(5);
                Build(result.second, distances.data(), 30);
                return result;
            }();
            Codes(out, tables.first, tables.second);
        }

        void Dynamic(std::vector<uint8_t>& out)
        {
            static constexpr uint8_t order[19]{ 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
            const int literalCount = Bits(5) + 257, distanceCount = Bits(5) + 1, codeCount = Bits(4) + 4;
            if (literalCount > 286 || distanceCount > 30) { failed = true; return; }
            std::array<uint8_t, 320> lengths{};
            for (int i = 0; i < codeCount; ++i) lengths[order[i]] = static_cast<uint8_t>(Bits(3));
            Huffman codes;
            Build(codes, lengths.data(), 19);
            lengths.fill(0);
            for (int i = 0; i < literalCount + distanceCount && !failed;)
            {
                const int symbol = Decode(codes);
                if (symbol < 16) { lengths[i++] = static_cast<uint8_t>(symbol); continue; }
                uint8_t value = 0;
                int repeat = 0;
                if (symbol == 16)
                {
                    if (!i) { failed = true; return; }
                    value = lengths[i - 1];
                    repeat = 3 + Bits(2);
                }
                else repeat = symbol == 17 ? 3 + Bits(3) : 11 + Bits(7);
                if (i + repeat > literalCount + distanceCount) { failed = true; return; }
                while (repeat--) lengths[i++] = value;
            }
            Huffman literals, distances;
            Build(literals, lengths.data(), literalCount);
            Build(distances, lengths.data() + literalCount, distanceCount);
            Codes(out, literals, distances);
        }
    };

    // An entry of an MT Framework archive (ARC, version 7)
    std::vector<uint8_t> ReadEntry(std::ifstream& file, const std::vector<uint8_t>& table, uint16_t count, std::string_view name)
    {
        constexpr uint32_t TextureType = 0x241F5DEB;
        for (uint16_t i = 0; i < count; ++i)
        {
            const auto entry = table.data() + i * 80;
            if (_strnicmp(reinterpret_cast<const char*>(entry), name.data(), name.size()) || entry[name.size()]) continue;
            uint32_t type, packed, unpacked, offset;
            std::memcpy(&type, entry + 64, 4);
            std::memcpy(&packed, entry + 68, 4);
            std::memcpy(&unpacked, entry + 72, 4);
            std::memcpy(&offset, entry + 76, 4);
            if (type != TextureType) continue;
            std::vector<uint8_t> compressed(packed);
            file.seekg(offset);
            if (!file.read(reinterpret_cast<char*>(compressed.data()), packed)) return {};
            std::vector<uint8_t> out;
            out.reserve(unpacked & 0x1FFFFFFF);
            if (!Inflater(compressed.data(), compressed.size()).Run(out)) return {};
            return out;
        }
        return {};
    }

    void DecodeBlock(const uint8_t* block, Pixel* out)
    {
        std::array<uint8_t, 8> alpha{ block[0], block[1] };
        if (alpha[0] > alpha[1])
            for (int i = 1; i < 7; ++i) alpha[i + 1] = static_cast<uint8_t>(((7 - i) * alpha[0] + i * alpha[1]) / 7);
        else
        {
            for (int i = 1; i < 5; ++i) alpha[i + 1] = static_cast<uint8_t>(((5 - i) * alpha[0] + i * alpha[1]) / 5);
            alpha[6] = 0;
            alpha[7] = 255;
        }
        uint64_t alphaBits = 0;
        std::memcpy(&alphaBits, block + 2, 6);
        const auto expand = [](uint16_t c)
        {
            Pixel p;
            p.r = static_cast<uint8_t>(((c >> 11) & 31) * 255 / 31);
            p.g = static_cast<uint8_t>(((c >> 5) & 63) * 255 / 63);
            p.b = static_cast<uint8_t>((c & 31) * 255 / 31);
            return p;
        };
        uint16_t c0, c1;
        std::memcpy(&c0, block + 8, 2);
        std::memcpy(&c1, block + 10, 2);
        std::array<Pixel, 4> colors{ expand(c0), expand(c1) };
        const auto mix = [](const Pixel& a, const Pixel& b)
        {
            return Pixel{ static_cast<uint8_t>((2 * a.r + b.r) / 3), static_cast<uint8_t>((2 * a.g + b.g) / 3),
                static_cast<uint8_t>((2 * a.b + b.b) / 3) };
        };
        colors[2] = mix(colors[0], colors[1]);
        colors[3] = mix(colors[1], colors[0]);
        uint32_t colorBits;
        std::memcpy(&colorBits, block + 12, 4);
        for (int i = 0; i < 16; ++i)
        {
            out[i] = colors[(colorBits >> (2 * i)) & 3];
            out[i].a = alpha[(alphaBits >> (3 * i)) & 7];
        }
    }

    void EncodeBlock(const Pixel* in, uint8_t* block)
    {
        // Alpha: eight steps between the extremes
        uint8_t low = 255, high = 0;
        for (int i = 0; i < 16; ++i)
        {
            low = std::min(low, in[i].a);
            high = std::max(high, in[i].a);
        }
        std::memset(block, 0, 16);
        block[0] = high;
        block[1] = low;
        if (high > low)
        {
            std::array<int, 8> steps{ high, low };
            for (int i = 1; i < 7; ++i) steps[i + 1] = ((7 - i) * high + i * low) / 7;
            uint64_t bits = 0;
            for (int i = 0; i < 16; ++i)
            {
                int best = 0;
                for (int s = 1; s < 8; ++s)
                    if (std::abs(steps[s] - in[i].a) < std::abs(steps[best] - in[i].a)) best = s;
                bits |= static_cast<uint64_t>(best) << (3 * i);
            }
            std::memcpy(block + 2, &bits, 6);
        }
        // Colour: the end points of the block's box diagonal that fits it best
        std::array<int, 3> minimum{ 255, 255, 255 }, maximum{ 0, 0, 0 };
        for (int i = 0; i < 16; ++i)
        {
            const int c[3]{ in[i].r, in[i].g, in[i].b };
            for (int k = 0; k < 3; ++k)
            {
                minimum[k] = std::min(minimum[k], c[k]);
                maximum[k] = std::max(maximum[k], c[k]);
            }
        }
        const auto pack = [](int r, int g, int b)
        {
            return static_cast<uint16_t>(((r * 31 + 127) / 255) << 11 | ((g * 63 + 127) / 255) << 5 | ((b * 31 + 127) / 255));
        };
        uint32_t bestError = UINT32_MAX;
        for (int signs = 0; signs < 4; ++signs)
        {
            const int g0 = signs & 1 ? minimum[1] : maximum[1], g1 = signs & 1 ? maximum[1] : minimum[1];
            const int b0 = signs & 2 ? minimum[2] : maximum[2], b1 = signs & 2 ? maximum[2] : minimum[2];
            uint16_t c0 = pack(maximum[0], g0, b0), c1 = pack(minimum[0], g1, b1);
            if (c0 < c1) std::swap(c0, c1);
            uint8_t candidate[16];
            std::memcpy(candidate, block, 8);
            std::memcpy(candidate + 8, &c0, 2);
            std::memcpy(candidate + 10, &c1, 2);
            std::memset(candidate + 12, 0, 4);
            Pixel palette[16];
            DecodeBlock(candidate, palette);
            // Palette entries: index i decodes to colours[i] for every pixel of an all-i block
            std::array<Pixel, 4> colors{};
            for (uint32_t index = 0; index < 4; ++index)
            {
                uint32_t all = 0;
                for (int i = 0; i < 16; ++i) all |= index << (2 * i);
                std::memcpy(candidate + 12, &all, 4);
                DecodeBlock(candidate, palette);
                colors[index] = palette[0];
            }
            uint32_t bits = 0, error = 0;
            for (int i = 0; i < 16; ++i)
            {
                uint32_t best = 0, bestDistance = UINT32_MAX;
                for (uint32_t index = 0; index < (c0 == c1 ? 1u : 4u); ++index)
                {
                    const int dr = colors[index].r - in[i].r, dg = colors[index].g - in[i].g, db = colors[index].b - in[i].b;
                    const uint32_t distance = dr * dr + dg * dg + db * db;
                    if (distance < bestDistance) { bestDistance = distance; best = index; }
                }
                bits |= best << (2 * i);
                error += bestDistance;
            }
            if (error < bestError)
            {
                bestError = error;
                std::memcpy(block + 8, &c0, 2);
                std::memcpy(block + 10, &c1, 2);
                std::memcpy(block + 12, &bits, 4);
            }
        }
    }

    // One word row of a language texture: the selected and the normal picture, and its letters
    struct Letter
    {
        char32_t character = 0;
        int left = 0, right = 0; // the columns of its strokes
        bool first = false, last = false; // at the start or end of its word
    };

    struct Row
    {
        std::vector<Pixel> selected, normal; // Width x Height each
        std::vector<Letter> letters;
        // The letter whose strokes are nearest to each pixel (-1: none within the glow) and the squared distance
        std::vector<int16_t> owner;
        std::vector<uint16_t> distance;
    };

    struct Library
    {
        std::vector<Row> rows;
        Pixel selectedBackground, normalBackground;
        float halfGap = 2.0f, centre = 245.5f;
    };

    // The words of the Option list in the textures of the title archive, row by row (fi and ffi are drawn as ligatures)
    struct LanguageWords
    {
        const char* folder;
        std::array<std::u32string_view, Rows> words;
    };

    constexpr LanguageWords sources[]
    {
        { "eng", { U"Display", U"Controls", U"Brightness", U"Audio", U"Language", U"Video settings", U"Quit Game",
            U"Auto data upload", U"Back to game" } },
        { "fre", { U"Aﬃchage", U"Commandes", U"Luminosité", U"Audio", U"Langue", U"Vidéo", U"Quitter",
            U"Téléchargement auto", U"Retour" } },
        { "ger", { U"Anzeige", U"Controller", U"Helligkeit", U"Audio", U"Sprache", U"Graﬁkeinstellungen",
            U"Spiel beenden", U"Auto-Datenübermittl.", U"Beenden" } },
        { "ita", { U"Formato schermo", U"Comandi", U"Luminosità", U"Audio", U"Lingua", U"Graﬁca", U"Esci dal gioco",
            U"Invio dati automatico", U"Indietro" } },
        { "spa", { U"Pantalla", U"Controles", U"Brillo", U"Sonido", U"Idioma", U"Opciones de vídeo", U"Salir del juego",
            U"Envío auto. de datos", U"Volver" } },
    };

    // Splits a row into letters by the columns of its strokes; a row whose letters don't match its text (a ligature)
    // is left out
    bool Measure(Row& row, std::u32string_view text)
    {
        const auto background = row.normal[20 * Width].a;
        std::vector<bool> stroke(Width * Height), column(Width);
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                if (row.normal[y * Width + x].a > background + 12)
                    stroke[y * Width + x] = column[x] = true;
        std::vector<std::pair<int, int>> spans;
        for (int x = 0; x < Width;)
        {
            if (!column[x]) { ++x; continue; }
            const int start = x;
            while (x < Width && column[x]) ++x;
            spans.emplace_back(start, x);
        }
        size_t n = 0;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == U' ') continue;
            if (n >= spans.size()) return false;
            Letter letter;
            letter.character = text[i];
            letter.left = spans[n].first;
            letter.right = spans[n].second;
            letter.first = i == 0 || text[i - 1] == U' ';
            letter.last = i + 1 == text.size() || text[i + 1] == U' ';
            row.letters.push_back(letter);
            ++n;
        }
        if (n != spans.size()) return false;
        // The nearest strokes of every pixel, from the stroke rows of the nearby columns
        std::vector<int16_t> columnOwner(Width, -1);
        for (size_t i = 0; i < row.letters.size(); ++i)
            for (int x = row.letters[i].left; x < row.letters[i].right; ++x) columnOwner[x] = static_cast<int16_t>(i);
        std::vector<std::vector<int>> strokeRows(Width);
        for (int x = 0; x < Width; ++x)
            for (int y = 0; y < Height; ++y)
                if (stroke[y * Width + x]) strokeRows[x].push_back(y);
        row.owner.assign(Width * Height, -1);
        row.distance.assign(Width * Height, UINT16_MAX);
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
            {
                int best = Spread * Spread + 1;
                for (int sx = std::max(0, x - Spread); sx <= std::min(Width - 1, x + Spread); ++sx)
                    for (const int sy : strokeRows[sx])
                    {
                        const int d = (sx - x) * (sx - x) + (sy - y) * (sy - y);
                        if (d < best)
                        {
                            best = d;
                            row.owner[y * Width + x] = columnOwner[sx];
                        }
                    }
                row.distance[y * Width + x] = static_cast<uint16_t>(best);
            }
        return true;
    }

    std::optional<Library> Load()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::ifstream file(std::filesystem::path(path).parent_path() / L"nativePC" / L"arcPC" / L"id" / L"e_rom" / L"title" / L"title.arc",
            std::ios::binary);
        char header[8]{};
        if (!file.read(header, 8) || std::memcmp(header, "ARC", 4)) return std::nullopt;
        uint16_t count;
        std::memcpy(&count, header + 6, 2);
        std::vector<uint8_t> table(count * 80);
        if (!file.read(reinterpret_cast<char*>(table.data()), table.size())) return std::nullopt;
        Library library;
        int gaps = 0, centres = 0;
        float gapSum = 0, centreSum = 0;
        for (const auto& source : sources)
        {
            const auto name = std::string("id\\e_rom\\pc\\tex\\") + source.folder + "\\option\\option01_ID_HQ";
            const auto texture = ReadEntry(file, table, count, name);
            if (texture.size() < 20) return std::nullopt;
            uint32_t offset;
            std::memcpy(&offset, texture.data() + 16, 4);
            if (texture.size() < offset + TextureSize * TextureSize) return std::nullopt;
            const auto blocks = texture.data() + offset;
            for (int k = 0; k < Rows; ++k)
            {
                Row row;
                row.selected.resize(Width * Height);
                row.normal.resize(Width * Height);
                for (int by = 0; by < Height / 4; ++by)
                    for (int bx = 0; bx < 2 * Width / 4; ++bx)
                    {
                        Pixel pixels[16];
                        DecodeBlock(blocks + ((FirstRow / 4 + k * Height / 4 + by) * (TextureSize / 4) + bx) * 16, pixels);
                        auto& target = bx < Width / 4 ? row.selected : row.normal;
                        const int x0 = (bx % (Width / 4)) * 4;
                        for (int i = 0; i < 16; ++i) target[(by * 4 + i / 4) * Width + x0 + i % 4] = pixels[i];
                    }
                if (!Measure(row, source.words[k])) continue;
                for (size_t i = 1; i < row.letters.size(); ++i)
                    if (!row.letters[i].first)
                    {
                        gapSum += row.letters[i].left - row.letters[i - 1].right;
                        ++gaps;
                    }
                centreSum += (row.letters.front().left + row.letters.back().right) * 0.5f;
                ++centres;
                if (library.rows.empty())
                {
                    library.selectedBackground = row.selected[20 * Width];
                    library.normalBackground = row.normal[20 * Width];
                }
                library.rows.push_back(std::move(row));
            }
        }
        if (!gaps || !centres) return std::nullopt;
        library.halfGap = gapSum / gaps * 0.5f;
        library.centre = centreSum / centres;
        return library;
    }

    // A run of letters of one source row that spells part of the word
    struct Piece
    {
        const Row* row;
        size_t first, count;
        float offset = 0;
    };

    std::u32string Decode(const std::string& text)
    {
        std::u32string out;
        for (size_t i = 0; i < text.size();)
        {
            const auto c = static_cast<uint8_t>(text[i]);
            const int length = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
            char32_t value = length == 1 ? c : c & (0x3F >> (length - 1));
            for (int k = 1; k < length && i + k < text.size(); ++k) value = (value << 6) | (text[i + k] & 0x3F);
            out.push_back(value);
            i += length;
        }
        return out;
    }

    // The fewest pieces that spell the word; pieces whose letters keep their own neighbours' spacing and a start and
    // end that were also a word's start and end come first
    std::optional<std::vector<Piece>> Spell(const Library& library, const std::u32string& word)
    {
        const size_t n = word.size();
        std::vector<float> cost(n + 1, 1e9f);
        std::vector<Piece> from(n + 1, Piece{ nullptr, 0, 0 });
        cost[0] = 0;
        for (size_t i = 0; i < n; ++i)
        {
            if (cost[i] >= 1e9f) continue;
            for (const auto& row : library.rows)
                for (size_t s = 0; s < row.letters.size(); ++s)
                    for (size_t length = 1; s + length <= row.letters.size() && i + length <= n; ++length)
                    {
                        const auto& last = row.letters[s + length - 1];
                        if (last.character != word[i + length - 1] || (length > 1 && row.letters[s + length - 2].last)) break;
                        const auto& first = row.letters[s];
                        const bool start = i == 0, end = i + length == n;
                        float c = cost[i] + 1;
                        if (!start && first.first) c += 1; // no spacing of its own on the left
                        if (!end && last.last) c += 1;
                        if (start && !first.first) c += 0.5f; // a neighbour's glow would have to be removed
                        if (end && !last.last) c += 0.5f;
                        if (c < cost[i + length])
                        {
                            cost[i + length] = c;
                            from[i + length] = Piece{ &row, s, length };
                        }
                    }
        }
        if (cost[n] >= 1e9f) return std::nullopt;
        std::vector<Piece> pieces;
        for (size_t i = n; i;)
        {
            pieces.insert(pieces.begin(), from[i]);
            i -= from[i].count;
        }
        return pieces;
    }

    // The selected and normal pictures of a word, centred in the row like the game's words
    std::optional<std::pair<std::vector<Pixel>, std::vector<Pixel>>> Compose(const Library& library, const std::string& text)
    {
        auto pieces = Spell(library, Decode(text));
        if (!pieces || pieces->empty()) return std::nullopt;
        const auto leftCut = [&](const Piece& p)
        {
            const auto& l = p.row->letters[p.first];
            return l.first ? l.left - library.halfGap : (p.row->letters[p.first - 1].right + l.left) * 0.5f;
        };
        const auto rightCut = [&](const Piece& p)
        {
            const auto& l = p.row->letters[p.first + p.count - 1];
            return l.last ? l.right + library.halfGap : (l.right + p.row->letters[p.first + p.count].left) * 0.5f;
        };
        float next = 0;
        for (size_t i = 0; i < pieces->size(); ++i)
        {
            auto& piece = (*pieces)[i];
            piece.offset = i ? next - leftCut(piece) : 0;
            next = rightCut(piece) + piece.offset;
        }
        const auto& head = pieces->front();
        const auto& tail = pieces->back();
        const float left = head.row->letters[head.first].left + head.offset;
        const float right = tail.row->letters[tail.first + tail.count - 1].right + tail.offset;
        const float shift = library.centre - (left + right) * 0.5f;
        std::vector<int> offsets;
        for (const auto& piece : *pieces) offsets.push_back(static_cast<int>(std::lround(piece.offset + shift)));
        std::pair<std::vector<Pixel>, std::vector<Pixel>> out{ std::vector<Pixel>(Width * Height, library.selectedBackground),
            std::vector<Pixel>(Width * Height, library.normalBackground) };
        // The top rows of a cell hold the glow of the descenders of the word above it in the texture
        for (int y = TopRows; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
            {
                int best = Spread * Spread + 1;
                for (size_t i = 0; i < pieces->size(); ++i)
                {
                    const auto& piece = (*pieces)[i];
                    const int sx = x - offsets[i];
                    if (sx < 0 || sx >= Width) continue;
                    const auto index = y * Width + sx;
                    const auto owner = piece.row->owner[index];
                    if (owner < static_cast<int>(piece.first) || owner >= static_cast<int>(piece.first + piece.count)) continue;
                    if (piece.row->distance[index] >= best) continue;
                    best = piece.row->distance[index];
                    out.first[y * Width + x] = piece.row->selected[index];
                    out.second[y * Width + x] = piece.row->normal[index];
                }
            }
        return out;
    }
}

export namespace REWord
{
    using namespace REWordParts;

    // Where the composed word is in the texture: the selected picture, then the normal one
    constexpr std::array<std::array<float, 4>, 2> Rectangles{ { { 0.0f, float(TargetRow), float(Width), float(TargetRow + Height) },
        { float(Width), float(TargetRow), float(2 * Width), float(TargetRow + Height) } } };

    enum class State { Pending, Ready, Failed };

    struct Request
    {
        IDirect3DTexture9* texture = nullptr;
        std::string word;
        std::atomic<State> state{ State::Pending };
    };

    namespace detail
    {
        inline std::mutex mutex;
        inline std::vector<std::shared_ptr<Request>> queue;
        inline std::shared_ptr<std::optional<Library>> library;
        inline std::atomic<bool> loaded{ false };
        inline std::map<std::string, std::vector<uint8_t>> words; // encoded blocks of a word, row by row

        inline const std::vector<uint8_t>* Blocks(const std::string& word)
        {
            const auto found = words.find(word);
            if (found != words.end()) return found->second.empty() ? nullptr : &found->second;
            auto& blocks = words[word];
            const auto pictures = *library ? Compose(**library, word) : std::nullopt;
            if (!pictures) return nullptr;
            constexpr int across = 2 * Width / 4;
            blocks.resize(across * (Height / 4) * 16);
            for (int by = 0; by < Height / 4; ++by)
                for (int bx = 0; bx < across; ++bx)
                {
                    const auto& source = bx < Width / 4 ? pictures->first : pictures->second;
                    const int x0 = (bx % (Width / 4)) * 4;
                    Pixel pixels[16];
                    for (int i = 0; i < 16; ++i) pixels[i] = source[(by * 4 + i / 4) * Width + x0 + i % 4];
                    EncodeBlock(pixels, blocks.data() + (by * across + bx) * 16);
                }
            return &blocks;
        }

        inline bool Write(IDirect3DDevice9* device, IDirect3DTexture9* texture, const std::vector<uint8_t>& blocks)
        {
            D3DSURFACE_DESC desc{};
            if (texture->GetLevelCount() != 1 || FAILED(texture->GetLevelDesc(0, &desc)) || desc.Format != D3DFMT_DXT5 ||
                desc.Width != TextureSize || desc.Height != TextureSize) return false;
            constexpr int across = 2 * Width / 4;
            const auto copy = [&](const D3DLOCKED_RECT& locked)
            {
                for (int by = 0; by < Height / 4; ++by)
                    std::memcpy(static_cast<uint8_t*>(locked.pBits) + by * locked.Pitch, blocks.data() + by * across * 16, across * 16);
            };
            RECT area{ 0, TargetRow, 2 * Width, TargetRow + Height };
            D3DLOCKED_RECT locked{};
            if (SUCCEEDED(texture->LockRect(0, &locked, &area, 0)))
            {
                copy(locked);
                texture->UnlockRect(0);
                return true;
            }
            // A texture in video memory: through a system memory copy of the area
            IDirect3DTexture9* staging = nullptr;
            if (FAILED(device->CreateTexture(2 * Width, Height, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &staging, nullptr))) return false;
            bool done = false;
            if (SUCCEEDED(staging->LockRect(0, &locked, nullptr, 0)))
            {
                copy(locked);
                staging->UnlockRect(0);
                IDirect3DSurface9* from = nullptr;
                IDirect3DSurface9* to = nullptr;
                if (SUCCEEDED(staging->GetSurfaceLevel(0, &from)) && SUCCEEDED(texture->GetSurfaceLevel(0, &to)))
                {
                    const POINT at{ 0, TargetRow };
                    done = SUCCEEDED(device->UpdateSurface(from, nullptr, to, &at));
                }
                if (from) from->Release();
                if (to) to->Release();
            }
            staging->Release();
            return done;
        }
    }

    // Reads the letters in the background, before the first menu needs them
    inline void Install()
    {
        detail::library = std::make_shared<std::optional<Library>>();
        std::thread([library = detail::library]()
        {
            *library = Load();
            detail::loaded = true;
        }).detach();
    }

    // The D3D texture an Option list picture draws from
    inline IDirect3DTexture9* TextureOf(void* picture)
    {
        constexpr uint32_t Dxt5 = MAKEFOURCC('D', 'X', 'T', '5');
        const auto read = [](const void* p, size_t offset) { return *reinterpret_cast<uint8_t* const*>(static_cast<const uint8_t*>(p) + offset); };
        auto entry = read(picture, 228);
        auto resource = entry ? read(entry, 0x28) : nullptr;
        auto native = resource ? read(resource, 0xD4) : nullptr;
        if (!native || *reinterpret_cast<const uint32_t*>(native + 0x30) != Dxt5) return nullptr;
        return *reinterpret_cast<IDirect3DTexture9**>(native + 0x34);
    }

    // Asks for the word's pictures in the texture; the render thread writes them
    inline std::shared_ptr<Request> Submit(IDirect3DTexture9* texture, const std::string& word)
    {
        auto request = std::make_shared<Request>();
        request->word = word;
        if (!texture)
        {
            request->state = State::Failed;
            return request;
        }
        texture->AddRef();
        request->texture = texture;
        const std::lock_guard lock(detail::mutex);
        detail::queue.push_back(request);
        return request;
    }

    // On the render thread, before Present
    inline void Frame(IDirect3DDevice9* device)
    {
        if (!detail::loaded) return;
        std::vector<std::shared_ptr<Request>> queue;
        {
            const std::lock_guard lock(detail::mutex);
            queue.swap(detail::queue);
        }
        for (const auto& request : queue)
        {
            const auto blocks = detail::Blocks(request->word);
            const bool written = blocks && detail::Write(device, request->texture, *blocks);
            request->texture->Release();
            request->texture = nullptr;
            request->state = written ? State::Ready : State::Failed;
        }
    }
}
