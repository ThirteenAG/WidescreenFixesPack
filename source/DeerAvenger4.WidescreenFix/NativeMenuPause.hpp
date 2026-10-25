#pragma once

#include "NativeMenuSession.hpp"
#include "NativeMenuContext.hpp"
#include "NativeMenuABI.hpp"

namespace DeerMenu
{
    struct Pause : std::enable_shared_from_this<Pause>
    {
        PauseContext context;
        HMODULE module = nullptr;
        void* font = nullptr;
        std::shared_ptr<Session> session;
        bool requested = false;
        bool retired = false;
        bool entryPainted = false;
        uint64_t paintedGeneration = 0, paintedSerial = 0;

        static void Write(uintptr_t address, int value)
        {
            *reinterpret_cast<int*>(address) = value;
        }

        bool Current() const
        {
            return !retired && font && context.Current();
        }

        bool Private() const
        {
            return Current() && session && session->open && Read<int>(0x5322B0) == 3;
        }

        void Retire() noexcept
        {
            retired = true;
            requested = false;
            if (session) session->Retire();
        }

        // Only the complete bridge's zero-depth/pre-engine-retirement scope
        // may call this; no C++/DLL-detach destructor invokes native APIs.
        void Release()
        {
            Retire();
            if (font)
            {
                const auto owned = font;
                font = nullptr;
                Native::DestroyTypeface(owned);
            }
        }

        bool Prepare(HMODULE owner)
        {
            auto lease = shared_from_this();
            module = owner;
            if (retired || !owner || !context.Current()) return false;
            if (!font)
            {
                // Same palette used by native diagnostic419190. The public
                // constructor copies these values into its own raster.
                const uint32_t foreground = 0x00ffffff;
                const uint32_t background = 0x00320000;
                font = Native::CreateTypeface(&foreground, &background);
            }
            if (!Current()) return false;
            if (!session)
            {
                session = std::make_shared<Session>();
                session->module = module;
                session->pause = true;
                session->width = context.width - 48;
                session->owner = [weak = std::weak_ptr<Pause>(lease)]
                {
                    const auto host = weak.lock();
                    return host && host->Current() && Read<int>(0x5322B0) == 3;
                };
            }
            return Current();
        }

        bool Key(int key)
        {
            auto lease = shared_from_this();
            if (!Current()) return false;
            const int nativeSelection = Read<int>(0x5322B0);
            if (Private())
            {
                const bool painted = session->ready && paintedGeneration == session->generation &&
                    paintedSerial == session->serial;
                // Back can cancel construction/unseen changes; every other
                // owned command waits for that exact native view to be drawn.
                if (key != 128 && !painted &&
                    (key == 147 || key == 148 || key == 149 || key == 150 || key == 171))
                    return true;
                if (key == 147 || key == 148)
                {
                    if (session->busy) session->Close();
                    else session->Move(key == 147 ? -1 : 1);
                    return true;
                }
                if (key == 128 || key == 171 || key == 149 || key == 150)
                    return session->Queue(key == 149 ? -1 : key == 150 ? 1 : 0,
                        key == 128, session->selected);
                // Do not let confirmation aliases become latent stock input.
                if (key == 121 || key == 110) return true;
                return false;
            }
            if (!entryPainted) return false;
            if (nativeSelection < 0 || nativeSelection > 3) return false;
            if (key == 147 || key == 148)
            {
                Write(0x5322B0, std::clamp(nativeSelection + (key == 147 ? -1 : 1), 0, 3));
                return true;
            }
            if (key == 171 && nativeSelection == 3)
            {
                requested = true;
                return true;
            }
            return false;
        }

        void Frame(const std::string& language)
        {
            auto lease = shared_from_this();
            if (!Current())
            {
                if (session) session->Close();
                requested = false;
                return;
            }
            if (requested)
            {
                requested = false;
                if (Read<int>(0x5322B0) == 3 && session && !session->open)
                    session->Open(language);
            }
            else if (session && session->open)
                session->Frame();
        }

        bool Draw()
        {
            auto lease = shared_from_this();
            if (!Current() || !context.Rendering()) return false;
            if (!Private())
            {
                const std::string caption = Read<int>(0x5322B0) == 3 ? "> Plugins" : "  Plugins";
                const int y = context.height - Native::GlyphHeight - 16;
                const int x = (context.width - int(caption.size()) * Native::GlyphWidth) / 2;
                const bool result = x >= 16 && y >= 16 && Native::DrawText(font, caption.c_str(), x, y) && Current();
                entryPainted = result;
                return result;
            }
            if (!session->ready || session->view.items.size() > 5 || session->view.headings.size() > 4)
                return false;
            const auto serial = session->serial;
            const auto generation = session->generation;
            const View snapshot = session->view;
            const size_t selected = session->selected;
            const auto current = [&] { return Private() && context.Rendering() &&
                session->Current(generation) && session->serial == serial; };
            std::string help;
            std::vector<std::string> helpLines;
            if (!session->ReadHelp(help) || !current() ||
                (!help.empty() && !SplitText(help, context.width - 32 - 2 * Native::GlyphWidth, helpLines)))
                return false;
            const auto lines = snapshot.headings.size() + snapshot.items.size() + helpLines.size();
            if (32 + 20 + int(lines) * (Native::GlyphHeight + 4) > context.height)
                return false;
            int y = 16;
            const auto line = [&](const std::string& text, bool focused)
            {
                const std::string display = focused ? "> " + text : "  " + text;
                if (!current() || !TextFits(display, context.width - 32, Native::GlyphHeight) ||
                    y + Native::GlyphHeight > context.height - 16) return false;
                const bool result = Native::DrawText(font, display.c_str(), 16, y);
                y += Native::GlyphHeight + 4;
                return result && current();
            };
            for (const auto& heading : snapshot.headings)
                if (!line(heading, false)) return false;
            y += 8;
            for (size_t i = 0; i < snapshot.items.size(); ++i)
                if (!line(snapshot.items[i].text, i == selected)) return false;
            y += 12;
            for (const auto& helpLine : helpLines)
                if (!line(helpLine, false)) return false;
            if (!current()) return false;
            paintedGeneration = generation;
            paintedSerial = serial;
            return true;
        }
    };
}
