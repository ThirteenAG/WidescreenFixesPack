#pragma once
#include "PSP.hpp"
#include "PortableIni.hpp"

namespace console::portable {
// Called after Start has selected this module's CFG. Preserve loaded values and
// unrelated lines; append every missing described default without INI migration.
template<size_t Count> bool CompleteCfg(const char* path, IniValue (&values)[Count]) {
    for (auto& value : values) {
        char buffer[sizeof(value.value)]{};
        auto loaded = inireader.ReadString(value.section, value.key, value.value, buffer, sizeof(buffer));
        if (loaded && loaded != value.value && std::strlen(loaded) < sizeof(value.value)) { std::strcpy(value.value, loaded); }
    }
    IniWriter<> writer;
    if (!writer.update(path, values, Count)) return false;
    inireader.SetIniPath(path);
    return true;
}
}
