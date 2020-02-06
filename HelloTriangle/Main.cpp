#include "D3D12HelloTriangle.h"
#include <Windows.h>
#include <string>

// Main message handler for the sample.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto pSample = reinterpret_cast<D3D12HelloTriangle *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
        // Save the DXSample* passed in to CreateWindow.
        auto pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        if (pSample)
        {
            pSample->Render(hWnd);
        }
        return 0;
    }

    case WM_SIZE:
    {
        pSample->SetSize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

struct CommandLine
{
    // Adapter info.
    bool m_useWarpDevice;

    // Window title.
    std::wstring m_title;

    // Helper function for parsing any supplied command line args.
    _Use_decl_annotations_ void ParseCommandLineArgs()
    {
        // Parse the command line parameters
        int argc;
        auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        for (int i = 1; i < argc; ++i)
        {
            if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
                _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
            {
                m_useWarpDevice = true;
                m_title = m_title + L" (WARP)";
            }
        }

        LocalFree(argv);
    }
};

_Use_decl_annotations_ int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    CommandLine cmd;
    cmd.ParseCommandLineArgs();

    D3D12HelloTriangle sample(cmd.m_useWarpDevice);

    // Initialize the window class.
    WNDCLASSEX windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    if (!RegisterClassEx(&windowClass))
    {
        return 1;
    }

    // Create the window and store a handle to it.
    auto hWnd = CreateWindow(
        windowClass.lpszClassName,
        cmd.m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr, // We have no parent window.
        nullptr, // We aren't using menus.
        hInstance,
        &sample);

    RECT rect;
    GetClientRect(hWnd, &rect);
    sample.SetSize(rect.right-rect.left, rect.bottom-rect.top);

    ShowWindow(hWnd, nCmdShow);

    // Main sample loop.
    MSG msg;
    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}
