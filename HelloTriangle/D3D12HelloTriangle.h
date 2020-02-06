#pragma once

class D3D12HelloTriangle
{
    class Impl *m_impl = nullptr;

public:
    D3D12HelloTriangle(bool useWarpDevice, unsigned int frameCount);
    ~D3D12HelloTriangle();
    void Render(void* hWnd);
    void SetSize(int w, int h);
};
