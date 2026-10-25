#pragma once
#include "NativeMenuContext.hpp"
#include "gameref.hpp"
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace HD2Menu
{
    template<class T> void Write(void* object, size_t offset, const T& value)
    {
        ProtectedGameRef<T>{ reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset) } = value;
    }

    struct CellSpec
    {
        std::string frame;
        void* callbackOwner = nullptr;
        ABI::Action accept = nullptr;
        uint32_t type = 20;
        uint32_t style = 13;
    };

    class Window
    {
    public:
        Window() = default;
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        ~Window() { Reset(); }

        bool Build(void* scene, void* sector, uint32_t index, const std::vector<CellSpec>& specs,
            const std::function<bool()>& current)
        {
            failure = "private window initial identity";
            if (widget || !index || index >= 64 || specs.empty() || specs.size() > 8 || !current()) return false;
            failure = "private window allocation";
            auto memory = ABI::Allocate(132);
            if (!memory || !current())
            {
                ABI::Free(memory);
                return false;
            }
            failure = "private window constructor";
            widget = ABI::ConstructWindow(memory, scene, sector);
            if (!widget)
            {
                ABI::Free(memory);
                return false;
            }
            failure = "private window name/current identity";
            if (!current() || !name.Assign("FusionFixHubAdvanced") || !current()) return false;
            ABI::Page page{};
            page.id = index << 26;
            page.name = name.Get();
            failure = "private window realization/table identity";
            if (!ABI::RealizeWindow(widget, page) || !current() || !Context::Table(widget, 0x815540)) return false;
            cells.resize(specs.size());
            for (size_t i = 0; i < specs.size(); ++i)
            {
                auto& cell = cells[i];
                failure = "private control name/current identity";
                if (!cell.name.Assign(specs[i].frame) || !current()) return false;
                ABI::Child child{};
                child.name = cell.name.Get();
                child.id = page.id | (static_cast<uint32_t>(i + 1) << 20);
                child.type = specs[i].type;
                std::array<ABI::Message, 2> properties{};
                size_t count = 0;
                if (specs[i].accept)
                {
                    failure = "private control callback identity";
                    if (!specs[i].callbackOwner || child.type != 20) return false;
                    auto& binding = properties[count++];
                    binding.event = 0x01000007;
                    binding.value = 0x04001000;
                    binding.owner = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(specs[i].callbackOwner));
                    binding.callback = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(specs[i].accept));
                }
                child.properties.begin = properties.data();
                child.properties.end = properties.data() + count;
                child.properties.capacity = child.properties.end;
                failure = "private control realization/current identity";
                ABI::RealizeControl(widget, sector, sector, child);
                if (!current()) return false;
                void** children = nullptr;
                size_t actual = 0;
                failure = "private control appended-child identity";
                if (!Context::Children(widget, children, actual) || actual != i + 1)
                {
                    auto frame = ABI::FindFrame(sector, specs[i].frame.c_str(), 65569);
                    if (!current()) failure = "private control failed/source identity changed";
                    else if (!frame) failure = "private control named frame lookup";
                    else if (!ABI::FindFrame(frame, "normal", 65537)) failure = "private control normal visual lookup";
                    else if (!ABI::FindFrame(frame, "actived", 65537)) failure = "private control active visual lookup";
                    return false;
                }
                cell.widget = children[i];
                uint32_t id = 0;
                failure = "private control table/identifier identity";
                if (!Context::Table(cell.widget, child.type == 20 ? 0x814CB0 : 0x813CC0)
                    || !Context::Read(cell.widget, 96, id) || id != child.id) return false;
                cell.id = id;
                cell.style = specs[i].style;
                if (child.type == 20)
                    ABI::Virtual<uint32_t(__thiscall*)(void*, uint32_t)>(cell.widget, 52)(cell.widget, id);
                failure = "private control focus/current identity";
                if (!current()) return false;
            }
            return true;
        }

        bool Text(size_t index, const std::string& value, bool enabled, const std::function<bool()>& current)
        {
            if (index >= cells.size() || !widget || !current()) return false;
            auto& cell = cells[index];
            void* text = nullptr;
            if (!Context::Read(cell.widget, 108, text) || !text) return false;
            ABI::String copied;
            if (!copied.Assign(value) || !current()) return false;
            if (!ABI::SetText(text, copied.Get(), cell.style) || !current()) return false;
            if (!ABI::FontStyle(text, cell.style) || !current()) return false;
            ABI::Message state{};
            //1000001 sets native disabled100;1000002 clears it.
            state.event = enabled ? 0x01000002 : 0x01000001;
            ABI::Send(cell.widget, state);
            return current();
        }

        void Adopt() { managerOwned = true; }
        void NativeDestroyed() { widget = nullptr; managerOwned = false; cells.clear(); }
        void Reset()
        {
            if (widget && !managerOwned) ABI::DestroyWidget(std::exchange(widget, nullptr));
            if (!widget) cells.clear();
        }
        void* Get() const { return widget; }
        const char* Failure() const { return failure; }
        bool Adopted() const { return managerOwned; }
        size_t Count() const { return cells.size(); }
        void* Cell(size_t index) const { return index < cells.size() ? cells[index].widget : nullptr; }
        uint32_t ID(size_t index) const { return index < cells.size() ? cells[index].id : 0; }

    private:
        struct CellData
        {
            ABI::String name;
            void* widget = nullptr;
            uint32_t id = 0;
            uint32_t style = 13;
        };
        void* widget = nullptr;
        const char* failure = "private native window/controls";
        bool managerOwned = false;
        ABI::String name;
        std::vector<CellData> cells;
    };
}
