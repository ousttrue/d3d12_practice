#include "D3D12HelloConstBuffers.h"
#include "ScreenState.h"

int WIDTH = 1280;
int HEIGHT = 720;
std::wstring CLASS_NAME(L"DXSampleClass");
std::wstring WINDOW_TITLE(L"D3D12 Hello Constant Buffers");

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

struct Gwlp
{
    // last param of CreateWindow to GWLP
    static void Set(HWND hWnd, LPARAM lParam)
    {
        auto pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }

    template <class T>
    static T *Get(HWND hWnd)
    {
        return reinterpret_cast<T *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
};

class Window
{
    HWND m_hwnd = NULL;
    ScreenState m_state;

public:
    HWND Create(const wchar_t *className, const wchar_t *titleName,
                int width, int height)
    {
        auto hInstance = GetModuleHandle(NULL);
        // Initialize the window class.
        WNDCLASSEX windowClass = {0};
        windowClass.cbSize = sizeof(WNDCLASSEX);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = hInstance;
        windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        windowClass.lpszClassName = className;
        if (!RegisterClassEx(&windowClass))
        {
            return NULL;
        }

        RECT windowRect = {0, 0, width, height};
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        // Create the window and store a handle to it.
        m_hwnd = CreateWindow(
            className,
            titleName,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr, // We have no parent window.
            nullptr, // We aren't using menus.
            hInstance,
            this);

        return m_hwnd;
    }

    void Show(int nCmdShow = SW_SHOW)
    {
        ShowWindow(m_hwnd, nCmdShow);
    }

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto window = Gwlp::Get<Window>(hWnd);

        switch (message)
        {
        case WM_CREATE:
            Gwlp::Set(hWnd, lParam);
            return 0;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                window->m_state.Width = LOWORD(lParam);
                window->m_state.Height = HIWORD(lParam);
            }
            break;

            // TODO:
            // case WM_KEYDOWN:
            //     if (pSample)
            //     {
            //         pSample->OnKeyDown(static_cast<UINT8>(wParam));
            //     }
            //     return 0;

            // case WM_KEYUP:
            //     if (pSample)
            //     {
            //         pSample->OnKeyUp(static_cast<UINT8>(wParam));
            //     }
            //     return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        // Handle any messages the switch statement didn't.
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    bool Update(ScreenState *pState)
    {
        MSG msg = {};
        while (true)
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                break;
            }
        }

        if (msg.message == WM_QUIT)
        {
            return false;
        }
        *pState = m_state;
        return true;
    }
};

_Use_decl_annotations_ int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    CommandLine cmd;
    cmd.Parse();

    Window window;
    auto hwnd = window.Create(CLASS_NAME.c_str(), WINDOW_TITLE.c_str(),
                              WIDTH, HEIGHT);
    if (!hwnd)
    {
        return 1;
    }
        window.Show();

    {
        D3D12HelloConstBuffers renderer(cmd.m_useWarpDevice);
        ScreenState state;
        while (window.Update(&state))
        {
            renderer.OnFrame(hwnd, state);
        }

        return 0;
    }
}
