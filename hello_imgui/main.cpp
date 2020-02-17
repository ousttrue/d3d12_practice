// dear imgui: standalone example application for DirectX 12
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// FIXME: 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*))

#include "D3DRenderer.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include <tchar.h>

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

static int const NUM_BACK_BUFFERS = 3;

static void EnableDebugLayer()
{
    ComPtr<ID3D12Debug> pdx12Debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
    {
        pdx12Debug->EnableDebugLayer();
    }
}

static void ReportLiveObjects()
{
    ComPtr<IDXGIDebug1> pDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
    }
}

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

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto renderer = Gwlp::Get<D3DRenderer>(hWnd);

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_CREATE:
        Gwlp::Set(hWnd, lParam);
        break;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            ImGui_ImplDX12_InvalidateDeviceObjects();
            renderer->OnSize(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
            ImGui_ImplDX12_CreateDeviceObjects();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

class WindowImGui
{
    HWND g_hWnd = nullptr;
    INT64 g_Time = 0;
    INT64 g_TicksPerSecond = 0;
    ImGuiMouseCursor g_LastMouseCursor = ImGuiMouseCursor_COUNT;

public:
    bool Init(HWND hwnd)
    {
        // ImGui_ImplWin32_Init(hwnd);
        if (!::QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond))
            return false;
        if (!::QueryPerformanceCounter((LARGE_INTEGER *)&g_Time))
            return false;

        // Setup back-end capabilities flags
        g_hWnd = (HWND)hwnd;
        ImGuiIO &io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendPlatformName = "imgui_impl_win32";
        io.ImeWindowHandle = hwnd;

        // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
        io.KeyMap[ImGuiKey_Tab] = VK_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
        io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
        io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
        io.KeyMap[ImGuiKey_Home] = VK_HOME;
        io.KeyMap[ImGuiKey_End] = VK_END;
        io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
        io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
        io.KeyMap[ImGuiKey_Space] = VK_SPACE;
        io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
        io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
        io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
        io.KeyMap[ImGuiKey_A] = 'A';
        io.KeyMap[ImGuiKey_C] = 'C';
        io.KeyMap[ImGuiKey_V] = 'V';
        io.KeyMap[ImGuiKey_X] = 'X';
        io.KeyMap[ImGuiKey_Y] = 'Y';
        io.KeyMap[ImGuiKey_Z] = 'Z';

        return true;
    }

    void UpdateMousePos()
    {
        ImGuiIO &io = ImGui::GetIO();

        // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos)
        {
            POINT pos = {(int)io.MousePos.x, (int)io.MousePos.y};
            ::ClientToScreen(g_hWnd, &pos);
            ::SetCursorPos(pos.x, pos.y);
        }

        // Set mouse position
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        POINT pos;
        if (HWND active_window = ::GetForegroundWindow())
            if (active_window == g_hWnd || ::IsChild(active_window, g_hWnd))
                if (::GetCursorPos(&pos) && ::ScreenToClient(g_hWnd, &pos))
                    io.MousePos = ImVec2((float)pos.x, (float)pos.y);
    }

    bool UpdateMouseCursor()
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
            return false;

        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            ::SetCursor(NULL);
        }
        else
        {
            // Show OS mouse cursor
            LPTSTR win32_cursor = IDC_ARROW;
            switch (imgui_cursor)
            {
            case ImGuiMouseCursor_Arrow:
                win32_cursor = IDC_ARROW;
                break;
            case ImGuiMouseCursor_TextInput:
                win32_cursor = IDC_IBEAM;
                break;
            case ImGuiMouseCursor_ResizeAll:
                win32_cursor = IDC_SIZEALL;
                break;
            case ImGuiMouseCursor_ResizeEW:
                win32_cursor = IDC_SIZEWE;
                break;
            case ImGuiMouseCursor_ResizeNS:
                win32_cursor = IDC_SIZENS;
                break;
            case ImGuiMouseCursor_ResizeNESW:
                win32_cursor = IDC_SIZENESW;
                break;
            case ImGuiMouseCursor_ResizeNWSE:
                win32_cursor = IDC_SIZENWSE;
                break;
            case ImGuiMouseCursor_Hand:
                win32_cursor = IDC_HAND;
                break;
            case ImGuiMouseCursor_NotAllowed:
                win32_cursor = IDC_NO;
                break;
            }
            ::SetCursor(::LoadCursor(NULL, win32_cursor));
        }
        return true;
    }

    void NewFrame()
    {
        // ImGui_ImplWin32_NewFrame();
        ImGuiIO &io = ImGui::GetIO();
        IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

        // Setup display size (every frame to accommodate for window resizing)
        RECT rect;
        ::GetClientRect(g_hWnd, &rect);
        io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

        // Setup time step
        INT64 current_time;
        ::QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
        io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
        g_Time = current_time;

        // Read keyboard modifiers inputs
        io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
        io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
        io.KeySuper = false;
        // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

        // Update OS mouse position
        UpdateMousePos();

        // Update OS mouse cursor with the cursor requested by imgui
        ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
        if (g_LastMouseCursor != mouse_cursor)
        {
            g_LastMouseCursor = mouse_cursor;
            UpdateMouseCursor();
        }

        // Update game controllers (if enabled and available)
        // ImGui_ImplWin32_UpdateGamepads();
    }
};

int run()
{
    D3DRenderer renderer(NUM_BACK_BUFFERS);

    // Create application window
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL};
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX12 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, &renderer);

    // Initialize Direct3D
    if (!renderer.CreateDeviceD3D(hwnd))
    {
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    WindowImGui imgui;
    imgui.Init(hwnd);
    ImGui_ImplDX12_Init(renderer.Device(), NUM_BACK_BUFFERS,
                        DXGI_FORMAT_R8G8B8A8_UNORM, renderer.SrvHeap(),
                        renderer.SrvHeap()->GetCPUDescriptorHandleForHeapStart(),
                        renderer.SrvHeap()->GetGPUDescriptorHandleForHeapStart());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        imgui.NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        {
            auto frameCtxt = renderer.Begin(&clear_color.x);
            ImGui::Render();
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderer.CommandList());
            renderer.End(frameCtxt);
        }
    }

    renderer.WaitForLastSubmittedFrame();
    ImGui_ImplDX12_Shutdown();
    ImGui::DestroyContext();

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

int main(int, char **)
{
#ifdef DX12_ENABLE_DEBUG_LAYER
    EnableDebugLayer();
#endif
    auto ret = run();
#ifdef DX12_ENABLE_DEBUG_LAYER
    ReportLiveObjects();
#endif
    return ret;
}
