#include "Game.hpp"
#include <cstring>
#include <psputility.h>

namespace mc3 {
namespace {
SafetyMipsInline movie;
SafetyMipsInline saveUpdate;
bool startup = true;

int UpdateStartupSave(uintptr_t manager) {
    // State 123 is the startup message dialog. Do not dismiss dialogs with
    // real Yes/No choices; let the game's normal shutdown handle the result.
    if (startup && at<int>(manager + 4) == 123 &&
        !(at<unsigned>(manager + 59488) & PSP_UTILITY_MSGDIALOG_OPTION_YESNO_BUTTONS) &&
        sceUtilityMsgDialogGetStatus() == 2)
        sceUtilityMsgDialogAbort();
    return saveUpdate.call<int>(manager);
}

int PlayStartupMovie(uintptr_t player, const char* name) {
    if (startup) {
        // These three calls form the engine's startup sequence. Returning the
        // native success value lets its state machine perform normal cleanup.
        if (std::strncmp(name, "legals_", 7) == 0 || std::strcmp(name, "rockstar") == 0)
            return 1;
        if (std::strcmp(name, "mcla_int") == 0) {
            startup = false;
            return 1;
        }
    }
    return movie.call<int>(player, name);
}
}
void InstallIntro() {
    if (!skipIntro) return;
    const auto site = pattern.get_first("70 FD BD 27 70 02 B0 AF 25 80 80 00 74 02 B1 AF ?? ?? 04 3C 25 88 A0 00 78 02 B2 AF", 0);
    movie = safetymips::create_inline(site, PlayStartupMovie);
    const auto saveSite = pattern.get_first("30 FE BD 27 B8 01 B0 AF 25 80 80 00 C0 01 B2 AF 04 00 12 8E 25 20 40 02", 0);
    saveUpdate = safetymips::create_inline(saveSite, UpdateStartupSave);
}
}
