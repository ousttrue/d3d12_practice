#include "CMainApplication.h"
#include "Commandline.h"

int main(int argc, char *argv[])
{
    CommandLine cmdline(argc, argv);

    CMainApplication pMainApplication(cmdline.m_nMSAASampleCount, cmdline.m_flSuperSampleScale, cmdline.m_iSceneVolumeInit);

    if (!pMainApplication.Initialize(cmdline.m_bDebugD3D12))
    {
        return 1;
    }

    pMainApplication.RunMainLoop();

    return 0;
}
