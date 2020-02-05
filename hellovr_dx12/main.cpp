#include "CMainApplication.h"
#include "Commandline.h"

int main(int argc, char *argv[])
{
    Commandline cmdline(argc, argv);

    CMainApplication pMainApplication(cmdline.m_nMSAASampleCount, cmdline.m_flSuperSampleScale);

    if (!pMainApplication.BInit(cmdline.m_bDebugD3D12, cmdline.m_iSceneVolumeInit))
    {
        return 1;
    }

    pMainApplication.RunMainLoop();

    return 0;
}
