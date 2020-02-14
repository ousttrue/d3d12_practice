#include "D3D12HelloConstBuffers.h"

int WIDTH = 1280;
int HEIGHT = 720;
std::wstring CLASS_NAME(L"DXSampleClass");
std::wstring WINDOW_TITLE(L"D3D12 Hello Constant Buffers");

// Main message handler for the sample.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto pSample = reinterpret_cast<D3D12HelloConstBuffers *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
        // Save the DXSample* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
        return 0;

    case WM_KEYDOWN:
        if (pSample)
        {
            pSample->OnKeyDown(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_KEYUP:
        if (pSample)
        {
            pSample->OnKeyUp(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_PAINT:
        if (pSample)
        {
            pSample->OnFrame();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

struct CommandLine
{
    bool m_useWarpDevice = false;
    // Window title.
    std::wstring m_title;

    void Parse()
    {
        // Parse the command line parameters
        int argc;
        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        ParseCommandLineArgs(argv, argc);
        LocalFree(argv);
    }

    // Helper function for parsing any supplied command line args.
    _Use_decl_annotations_ void ParseCommandLineArgs(WCHAR * argv[], int argc)
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

    D3D12HelloConstBuffers sample(WIDTH, HEIGHT, WINDOW_TITLE);
    // return Win32Application::Run(&sample, hInstance, nCmdShow, cmd.m_title.c_str(), cmd.m_useWarpDevice);

    // Initialize the window class.
    WNDCLASSEX windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = CLASS_NAME.c_str();
    RegisterClassEx(&windowClass);

    RECT windowRect = {0, 0, WIDTH, HEIGHT};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    auto hwnd = CreateWindow(
        windowClass.lpszClassName,
        WINDOW_TITLE.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, // We have no parent window.
        nullptr, // We aren't using menus.
        hInstance,
        &sample);

    // Initialize the sample. OnInit is defined in each child-implementation of DXSample.
    sample.OnInit(hwnd, cmd.m_useWarpDevice);

    ShowWindow(hwnd, nCmdShow);

    // Main sample loop.
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}
