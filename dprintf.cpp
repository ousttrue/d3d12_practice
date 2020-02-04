#include "dprintf.h"
#include <string.h>
#include <stdio.h>
#include <Windows.h>

static bool g_bPrintf = true;

void enablePrintf(bool enable)
{
    g_bPrintf = enable;
}

//-----------------------------------------------------------------------------
// Purpose: Outputs a set of optional arguments to debugging output, using
//          the printf format setting specified in fmt*.
//-----------------------------------------------------------------------------
void dprintf(const char *fmt, ...)
{
    va_list args;
    char buffer[2048];

    va_start(args, fmt);
    vsprintf_s(buffer, fmt, args);
    va_end(args);

    if (g_bPrintf)
        printf("%s", buffer);

    OutputDebugStringA(buffer);
}
