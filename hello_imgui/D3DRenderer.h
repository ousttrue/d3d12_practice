#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <vector>

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

struct FrameContext;
class D3DRenderer
{
    class Impl *m_impl = nullptr;

public:
    D3DRenderer(int frameCount);
    ~D3DRenderer();
    ID3D12Device *Device();
    ID3D12DescriptorHeap *SrvHeap();
    ID3D12GraphicsCommandList *CommandList();
    bool CreateDeviceD3D(HWND hWnd);
    void OnSize(HWND hWnd, UINT w, UINT h);
    void WaitForLastSubmittedFrame();
    void CleanupDeviceD3D();
    FrameContext *Begin(const float *clear_color);
    void End(FrameContext *frameCtxt);
};
