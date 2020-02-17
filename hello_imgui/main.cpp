// dear imgui: standalone example application for DirectX 12
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// FIXME: 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*))

#include "D3DRenderer.h"
#include "Window.h"
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

// ImGui_ImplDX12_InvalidateDeviceObjects();
// ImGui_ImplDX12_CreateDeviceObjects();
// if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
// Win32 message handler
// extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

    void NewFrame(HWND hWnd, const ScreenState &state)
    {
        // ImGui_ImplWin32_NewFrame();
        ImGuiIO &io = ImGui::GetIO();
        IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

        ////////////////////////////////////////////////////////////
        io.MousePos = ImVec2(state.X, state.Y);
        io.MouseDown[0] = state.Has(MouseButtonFlags::LeftDown);
        io.MouseDown[1] = state.Has(MouseButtonFlags::RightDown);
        io.MouseDown[2] = state.Has(MouseButtonFlags::MiddleDown);
        if (state.Has(MouseButtonFlags::WheelPlus))
        {
            io.MouseWheel = 1;
        }
        else if (state.Has(MouseButtonFlags::WheelMinus))
        {
            io.MouseWheel = -1;
        }
        else
        {
            io.MouseWheel = 0;
        }
        if (state.Has(MouseButtonFlags::CurosrChange))
        {
            if (!UpdateMouseCursor())
            {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
        }
        ////////////////////////////////////////////////////////////

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
        // UpdateMousePos(state);
        // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos)
        {
            POINT pos = {(int)io.MousePos.x, (int)io.MousePos.y};
            ::ClientToScreen(g_hWnd, &pos);
            ::SetCursorPos(pos.x, pos.y);
        }


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

    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui::GetCurrentContext() == NULL)
            return 0;

        ImGuiIO &io = ImGui::GetIO();
        switch (msg)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        {
            int button = 0;
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
            {
                button = 0;
            }
            if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK)
            {
                button = 1;
            }
            if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK)
            {
                button = 2;
            }
            if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK)
            {
                button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;
            }
            if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
                ::SetCapture(hwnd);
            io.MouseDown[button] = true;
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            int button = 0;
            if (msg == WM_LBUTTONUP)
            {
                button = 0;
            }
            if (msg == WM_RBUTTONUP)
            {
                button = 1;
            }
            if (msg == WM_MBUTTONUP)
            {
                button = 2;
            }
            if (msg == WM_XBUTTONUP)
            {
                button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;
            }
            io.MouseDown[button] = false;
            if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
                ::ReleaseCapture();
            return 0;
        }
        case WM_MOUSEWHEEL:
            io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
            return 0;
        case WM_MOUSEHWHEEL:
            io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam < 256)
                io.KeysDown[wParam] = 1;
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam < 256)
                io.KeysDown[wParam] = 0;
            return 0;
        case WM_CHAR:
            // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
            io.AddInputCharacter((unsigned int)wParam);
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT && UpdateMouseCursor())
                return 1;
            return 0;
            // case WM_DEVICECHANGE:
            //     if ((UINT)wParam == DBT_DEVNODES_CHANGED)
            //         g_WantUpdateHasGamepad = true;
            //     return 0;
        }
        return 0;
    }
};

int run()
{
    Window window(L"ImGui Example");
    auto hwnd = window.Create(L"Dear ImGui DirectX12 Example", 1280, 800);
    if (!hwnd)
    {
        return 1;
    }

    D3DRenderer renderer(NUM_BACK_BUFFERS);
    if (!renderer.CreateDeviceD3D(hwnd))
    {
        return 2;
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
    ScreenState state;
    while (window.Update(&state))
    {
        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        imgui.NewFrame(hwnd, state);
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
