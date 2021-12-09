#include "log.h"
#include <string.h>
#include <pspsdk.h>
#include <pspkernel.h>

#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"

void Write(char* path, char* message)
{
    if (logger.init == 0)
    {
        SceUID log_uid = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
        if (log_uid >= 0) sceIoClose(log_uid);
        logger.init = 1;
    }

    SceUID log_uid = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    sceIoWrite(log_uid, message, strlen(message));
    if (log_uid >= 0) sceIoClose(log_uid);
}

struct logger_t logger =
{
    .init = 0,
    .Write = Write
};