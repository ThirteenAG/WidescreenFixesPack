#pragma once
#include <algorithm>
#include <cstddef>

namespace RWGTAMenu
{
    // Fixed native lists reserve two rows for navigation only when the list needs another screen.
    class RootPages
    {
    public:
        static constexpr int Previous{ -1 };
        static constexpr int Next{ -2 };

        void Reset(size_t total, size_t capacity, size_t selected = 0)
        {
            totalRows = total;
            pageRows = total <= capacity ? capacity : capacity > 2 ? capacity - 2 : 0;
            offset = pageRows && selected < total ? selected / pageRows * pageRows : 0;
        }

        size_t Count() const
        {
            return pageRows ? std::min(pageRows, totalRows - offset) : 0;
        }

        size_t Rows() const
        {
            return pageRows ? Count() + (offset > 0) + (offset + Count() < totalRows) : 0;
        }

        int Page(size_t row) const
        {
            if (row < Count())
                return int(offset + row);
            row -= Count();
            if (offset > 0)
            {
                if (row == 0)
                    return Previous;
                --row;
            }
            return row == 0 && offset + Count() < totalRows ? Next : -3;
        }

        bool Move(int page)
        {
            if (!pageRows)
                return false;
            if (page == Previous && offset > 0)
                offset -= pageRows;
            else if (page == Next && offset + Count() < totalRows)
                offset += pageRows;
            else
                return false;
            return true;
        }

        size_t Selection(size_t page) const
        {
            return page >= offset && page < offset + Count() ? page - offset : 0;
        }

    private:
        size_t totalRows{};
        size_t pageRows{};
        size_t offset{};
    };
}
