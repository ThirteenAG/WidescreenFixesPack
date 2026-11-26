#pragma once
#include "PSP.hpp"
#include "PortableIni.hpp"

namespace console::portable {
// Called after Start has selected this module's CFG. Preserve loaded values and
// unrelated lines; append every missing described default without INI migration.
template<size_t Count> bool CompleteCfg(const char* path, const IniValue (&values)[Count]) {
    IniWriter<> writer;
    const bool completed = writer.update(path, values, Count, IniUpdate::AddMissing);
    inireader.SetIniPath(path);
    return completed;
}
}
