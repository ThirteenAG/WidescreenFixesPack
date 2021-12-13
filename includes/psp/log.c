#include "log.h"
#include <string.h>
#include <pspsdk.h>
#include <pspkernel.h>

#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"

void SetPath(const char* szFileName)
{
    logger.path = szFileName;
}

void Write(char* message)
{
    if (logger.log_uid == 0)
    {
        logger.log_uid = sceIoOpen(logger.path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
        if (logger.log_uid >= 0) sceIoClose(logger.log_uid);
        logger.log_uid = sceIoOpen(logger.path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    }
    sceIoWrite(logger.log_uid, message, strlen(message));
}

void WriteFormatted(const char* format, ...)
{
    char buffer[100];
    va_list val;
    int rv;
    va_start(val, format);
    rv = npf_vsnprintf(buffer, sizeof(buffer), format, val);
    logger.Write(buffer);
    va_end(val);
}

void Close()
{
    if (logger.log_uid >= 0) sceIoClose(logger.log_uid);
}

struct logger_t logger =
{
    .log_uid = 0,
    .path = 0,
    .SetPath = SetPath,
    .Write = Write,
    .WriteF = WriteFormatted,
    .Close = Close
};