#include "Commandline.h"
#include "dprintf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Commandline::Commandline(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (!_stricmp(argv[i], "-dxdebug"))
        {
            m_bDebugD3D12 = true;
        }
        else if (!_stricmp(argv[i], "-verbose"))
        {
            m_bVerbose = true;
        }
        else if (!_stricmp(argv[i], "-novblank"))
        {
            m_bVblank = false;
        }
        else if (!_stricmp(argv[i], "-msaa") && (argc > i + 1) && (*argv[i + 1] != '-'))
        {
            m_nMSAASampleCount = atoi(argv[i + 1]);
            i++;
        }
        else if (!_stricmp(argv[i], "-supersample") && (argc > i + 1) && (*argv[i + 1] != '-'))
        {
            m_flSuperSampleScale = (float)atof(argv[i + 1]);
            i++;
        }
        else if (!_stricmp(argv[i], "-noprintf"))
        {
            enablePrintf(false);
        }
        else if (!_stricmp(argv[i], "-cubevolume") && (argc > i + 1) && (*argv[i + 1] != '-'))
        {
            m_iSceneVolumeInit = atoi(argv[i + 1]);
            i++;
        }
    }
}
