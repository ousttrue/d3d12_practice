#pragma once
#include <string>
#include <Windows.h>

class D3D12HelloConstBuffers
{
    class Impl *m_impl=nullptr;

public:
    D3D12HelloConstBuffers(UINT width, UINT height);
    ~D3D12HelloConstBuffers();
    void OnInit(HWND hwnd, bool useWarpDevice);
    void OnFrame();
    void OnKeyDown(UINT8){}
    void OnKeyUp(UINT8){}
};
