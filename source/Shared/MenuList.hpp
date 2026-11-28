#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

// A list of menu rows shown through a fixed number of slots: the selection, the first row shown, and what Up/Down,
// a page key, the wheel, the scroll indicators and the mouse do. Nothing here knows the game: the host gives the rows
// (by a key that stays the same while the row exists, e.g. its Row pointer), draws row First() + s in slot s and
// passes its input on. Every list of a menu (plugins, categories, settings) is one continuous list; nothing pages.
namespace MenuList
{
    using Key = uintptr_t;

    class Window
    {
    public:
        // The number of rows the host shows at once
        void SetSlots(size_t count)
        {
            slots = std::max<size_t>(1, count);
            Follow();
        }

        size_t Slots() const { return slots; }

        // The rows now: only the shown ones, in their order. The selection stays on its row, and in its slot, when
        // rows above it hide or show; when its row is gone, the row that took its place is selected.
        void Update(const std::vector<Key>& rows)
        {
            const size_t slot = selected >= first ? selected - first : 0;
            size_t index = rows.empty() ? 0 : std::min(selected, rows.size() - 1);
            if (keyed)
            {
                auto it = std::find(rows.begin(), rows.end(), key);
                if (it != rows.end())
                    index = size_t(it - rows.begin());
            }
            keys = rows;
            selected = index;
            first = selected >= slot ? selected - slot : 0;
            Follow();
        }

        size_t Count() const { return keys.size(); }
        size_t Selected() const { return selected; }
        Key SelectedKey() const { return selected < keys.size() ? keys[selected] : 0; }
        size_t First() const { return first; }
        size_t Slot() const { return selected - first; }
        // Rows in the window (fewer than the slots at the end of a short list)
        size_t Shown() const { return std::min(slots, keys.size() - first); }
        // The row in a slot; Count() or more: the slot is empty
        size_t RowAt(size_t slot) const { return first + slot; }
        Key KeyAt(size_t slot) const { return first + slot < keys.size() ? keys[first + slot] : 0; }
        bool Above() const { return first > 0; }
        bool Below() const { return first + slots < keys.size(); }

        // Up (-1) or Down (+1): the next row, the window following by one row; at an end it wraps when wrap is set
        // (as the game's own lists do) and stays otherwise. True when the selection changed.
        bool Move(int direction, bool wrap = true)
        {
            if (keys.empty() || !direction)
                return false;
            size_t count = keys.size();
            size_t next = selected;
            if (direction > 0)
                next = selected + 1 < count ? selected + 1 : (wrap ? 0 : selected);
            else
                next = selected > 0 ? selected - 1 : (wrap ? count - 1 : selected);
            return Choose(next);
        }

        // Page Up/Page Down (or shoulder buttons): a window of rows, stopping at the ends
        bool Page(int direction)
        {
            if (keys.empty() || !direction)
                return false;
            size_t step = std::max<size_t>(1, slots - 1);
            size_t next = direction > 0 ? std::min(keys.size() - 1, selected + step) : (selected > step ? selected - step : 0);
            if (next == selected)
                return false;
            // The window moves by the same amount, so the selection keeps its slot where it can
            size_t slot = Slot();
            selected = next;
            first = selected >= slot ? selected - slot : 0;
            Remember();
            Follow();
            return true;
        }

        // Home (-1) or End (+1)
        bool Edge(int direction)
        {
            if (keys.empty() || !direction)
                return false;
            return Choose(direction > 0 ? keys.size() - 1 : 0);
        }

        // The wheel or a scroll indicator: the window moves by one row and the selection stays on its row, unless it
        // would leave the window, then it moves along. True when the window moved.
        bool Scroll(int direction)
        {
            if (!direction)
                return false;
            size_t last = keys.size() > slots ? keys.size() - slots : 0;
            size_t next = direction > 0 ? std::min(last, first + 1) : (first > 0 ? first - 1 : 0);
            if (next == first)
                return false;
            first = next;
            selected = std::clamp(selected, first, first + slots - 1);
            Remember();
            return true;
        }

        // The mouse over a slot (call it only while the mouse moves, so it never fights the keys): selects that row.
        // True when the selection changed; a slot past the end selects nothing.
        bool Point(size_t slot)
        {
            size_t row = first + slot;
            if (slot >= slots || row >= keys.size() || row == selected)
                return false;
            selected = row;
            Remember();
            return true;
        }

        // A row by its index, e.g. the row a list was left from; the window shows it
        void Select(size_t index)
        {
            selected = keys.empty() ? 0 : std::min(index, keys.size() - 1);
            Remember();
            Follow();
        }

        // A row by its key; false when it isn't in the list
        bool SelectKey(Key row)
        {
            auto it = std::find(keys.begin(), keys.end(), row);
            if (it == keys.end())
                return false;
            Select(size_t(it - keys.begin()));
            return true;
        }

        // The list's top, as when it is entered
        void Reset()
        {
            first = selected = 0;
            Remember();
        }

    private:
        std::vector<Key> keys;
        size_t slots{ 1 }, first{ 0 }, selected{ 0 };
        Key key{ 0 };
        bool keyed = false;

        void Remember()
        {
            keyed = selected < keys.size();
            key = keyed ? keys[selected] : 0;
        }

        bool Choose(size_t next)
        {
            if (next == selected)
                return false;
            selected = next;
            Remember();
            Follow();
            return true;
        }

        // The selection inside the window, the window inside the list, scrolled by as little as needed
        void Follow()
        {
            size_t count = keys.size();
            if (!count)
            {
                first = selected = 0;
                return;
            }
            selected = std::min(selected, count - 1);
            if (selected < first)
                first = selected;
            else if (selected >= first + slots)
                first = selected - slots + 1;
            size_t last = count > slots ? count - slots : 0;
            first = std::min(first, last);
            Remember();
        }
    };

    // Auto-repeat for a held direction, for hosts that read keys or pad buttons themselves: the first frame it is down,
    // then after the delay every interval (set them to the game's own menu rate)
    struct Repeat
    {
        uint32_t delay{ 400 }, interval{ 100 };

        // True on the frames the held input acts; now: a millisecond clock (GetTickCount64). first: the press itself
        // acts too (false when the game's own menu already acts on the press and only the repeats are added)
        bool Step(bool down, uint64_t now, bool first = true)
        {
            if (!down)
            {
                held = false;
                return false;
            }
            if (!held)
            {
                held = true;
                next = now + delay;
                return first;
            }
            if (now < next)
                return false;
            next += interval;
            if (next < now)
                next = now + interval;
            return true;
        }

    private:
        bool held = false;
        uint64_t next{ 0 };
    };

    // A stick axis as a menu direction: -1, 0 or +1 past the deadzone, with a smaller release threshold so a stick
    // resting near the edge of the deadzone doesn't flicker (no drift scrolling)
    struct StickAxis
    {
        int press{ 16000 }, release{ 12000 };   // XInput units (32767 full)

        int Direction(int value)
        {
            int magnitude = std::abs(value);
            int sign = value < 0 ? -1 : 1;
            if (current && (magnitude < release || sign != current))
                current = 0;
            if (!current && magnitude >= press)
                current = sign;
            return current;
        }

    private:
        int current = 0;
    };

    // Wheel deltas to notches: whole notches (120) are taken, the remainder is kept for the next deltas, so high-rate
    // mice that report fractions of a notch lose and double nothing. Positive: away from the user (up).
    struct WheelNotches
    {
        int Take(int delta)
        {
            rest += delta;
            int notches = rest / 120;
            rest -= notches * 120;
            return notches;
        }

        void Clear() { rest = 0; }

    private:
        int rest = 0;
    };
}
