#include "D3D12HelloConstBuffers.h"
#include "Window.h"

struct CommandLine
{
    bool m_useWarpDevice = false;
    std::wstring m_title = L"SAMPLE_TITLE";

    void Parse()
    {
        // Parse the command line parameters
        int argc;
        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        ParseCommandLineArgs(argv, argc);
        LocalFree(argv);
    }

    // Helper function for parsing any supplied command line args.
    _Use_decl_annotations_ void ParseCommandLineArgs(WCHAR *argv[], int argc)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
                _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
            {
                m_useWarpDevice = true;
                m_title = m_title + L" (WARP)";
            }
        }
    }
};

_Use_decl_annotations_ int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    CommandLine cmd;
    cmd.Parse();

    // create window
    Window window;
    auto hwnd = window.Create(L"SAMLE_CLASS", cmd.m_title.c_str(),
                              1280, 720);
    if (!hwnd)
    {
        return 1;
    }
    window.Show();

    {
        // main loop scope
        D3D12HelloConstBuffers renderer(cmd.m_useWarpDevice);
        ScreenState state;
        while (window.Update(&state))
        {
            renderer.OnFrame(hwnd, state);
        }
    }

    return 0;
}
