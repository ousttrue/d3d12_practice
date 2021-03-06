#pragma once
#include <string>
#include <Windows.h>

class D3D12HelloConstBuffers
{
    bool m_useWarpDevice;
    class Impl *m_impl=nullptr;

public:
    D3D12HelloConstBuffers(bool useWarpDevice);
    ~D3D12HelloConstBuffers();
    void OnFrame(void *hwnd, const struct ScreenState &state);
};
