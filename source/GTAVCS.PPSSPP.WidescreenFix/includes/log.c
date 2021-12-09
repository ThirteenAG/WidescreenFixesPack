#include "log.h"
#include <string.h>
#include <pspsdk.h>
#include <pspkernel.h>

#define NANOPRINTF_IMPLEMENTATION
#include "includes/nanoprintf.h"

void Write(char* path, char* message)
{
    if (logger.log_uid == 0)
    {
        logger.log_uid = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
        if (logger.log_uid >= 0) sceIoClose(logger.log_uid);
        logger.log_uid = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    }
    sceIoWrite(logger.log_uid, message, strlen(message));
}

void Close()
{
    if (logger.log_uid >= 0) sceIoClose(logger.log_uid);
}

struct logger_t logger =
{
    .log_uid = 0,
    .Write = Write,
    .Close = Close
};