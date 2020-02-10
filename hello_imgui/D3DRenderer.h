#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

struct FrameContext
{
    ID3D12CommandAllocator *CommandAllocator;
    UINT64 FenceValue;
};
class D3DRenderer
{
public:
    static int const NUM_BACK_BUFFERS = 3;
    static int const NUM_FRAMES_IN_FLIGHT = 3;

private:
    FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
    UINT g_frameIndex = 0;

    ID3D12Device *g_pd3dDevice = NULL;
    ID3D12DescriptorHeap *g_pd3dRtvDescHeap = NULL;
    ID3D12DescriptorHeap *g_pd3dSrvDescHeap = NULL;
    ID3D12CommandQueue *g_pd3dCommandQueue = NULL;
    ID3D12GraphicsCommandList *g_pd3dCommandList = NULL;
    ID3D12Fence *g_fence = NULL;
    HANDLE g_fenceEvent = NULL;
    UINT64 g_fenceLastSignaledValue = 0;
    IDXGISwapChain3 *g_pSwapChain = NULL;
    HANDLE g_hSwapChainWaitableObject = NULL;
    ID3D12Resource *g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
    D3D12_RESOURCE_BARRIER barrier = {};

public:
    ID3D12Device *Device() { return g_pd3dDevice; }
    ID3D12DescriptorHeap *SrvHeap() { return g_pd3dSrvDescHeap; }
    ID3D12GraphicsCommandList *CommandList() { return g_pd3dCommandList; }
    void OnSize(HWND hWnd, UINT w, UINT h);
    bool CreateDeviceD3D(HWND hWnd);
    void WaitForLastSubmittedFrame();
    void CleanupDeviceD3D();
    FrameContext *Begin(const float *clear_color);
    void End(FrameContext *frameCtxt);
private:
    FrameContext *WaitForNextFrameResources();
    void CleanupRenderTarget();
    void ResizeSwapChain(HWND hWnd, int width, int height);
    void CreateRenderTarget();
};
