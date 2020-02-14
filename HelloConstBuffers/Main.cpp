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
#include "D3D12HelloConstBuffers.h"
#include "Win32Application.h"

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

_Use_decl_annotations_ int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    CommandLine cmd;
    cmd.Parse();

    D3D12HelloConstBuffers sample(1280, 720, L"D3D12 Hello Constant Buffers");
    return Win32Application::Run(&sample, hInstance, nCmdShow, cmd.m_title.c_str(), cmd.m_useWarpDevice);
}
