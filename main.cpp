#include "CMainApplication.h"

int main(int argc, char *argv[])
{
    CMainApplication *pMainApplication = new CMainApplication(argc, argv);

    if (!pMainApplication->BInit())
    {
        pMainApplication->Shutdown();
        return 1;
    }

    pMainApplication->RunMainLoop();

    pMainApplication->Shutdown();

    delete pMainApplication;

    return 0;
}
