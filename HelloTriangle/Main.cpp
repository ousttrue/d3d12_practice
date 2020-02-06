//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "D3D12HelloTriangle.h"

// Main message handler for the sample.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto pSample = reinterpret_cast<D3D12HelloTriangle *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

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
            pSample->OnUpdate();
            pSample->OnRender();
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
    D3D12HelloTriangle sample(1280, 720, L"D3D12 Hello Triangle");

    CommandLine cmd;
    cmd.ParseCommandLineArgs();

    // Initialize the window class.
    WNDCLASSEX windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    RegisterClassEx(&windowClass);

    RECT windowRect = {0, 0, static_cast<LONG>(sample.GetWidth()), static_cast<LONG>(sample.GetHeight())};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    auto hWnd = CreateWindow(
        windowClass.lpszClassName,
        cmd.m_title.c_str(),
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
    sample.OnInit(hWnd, cmd.m_useWarpDevice);

    ShowWindow(hWnd, nCmdShow);

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
