#include "CMainApplication.h"

int main(int argc, char *argv[])
{
    CMainApplication pMainApplication(argc, argv);

    if (!pMainApplication.BInit())
    {
        return 1;
    }

    pMainApplication.RunMainLoop();

    return 0;
}
