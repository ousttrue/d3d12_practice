#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <vector>
#include "d3dx12.h"
#include "util.h"

class Swapchain
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<IDXGISwapChain3> m_swapChain;
    UINT m_frameIndex = 0;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    UINT m_rtvDescriptorSize = 0;
    std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
    UINT m_frameCount;
    HANDLE m_swapChainEvent = NULL;

public:
    Swapchain(UINT frameCount)
        : m_frameCount(frameCount)
    {
    }
    ~Swapchain()
    {
    }
    void Initialize(const ComPtr<IDXGIFactory4> &factory,
                    const ComPtr<ID3D12CommandQueue> &queue,
                    HWND hWnd, UINT w, UINT h);
    void CreateRenderTargets(const ComPtr<ID3D12Device> &device);
    void Present();
    // void Wait();

public:
    const ComPtr<ID3D12Resource> &CurrentRTV() const
    {
        return m_renderTargets[m_frameIndex];
    }

    D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
    const D3D12_CPU_DESCRIPTOR_HANDLE &CurrentHandle()
    {
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
        m_handle = handle;
        return m_handle;
    }

private:
    void UpdateFrameIndex()
    {
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    UINT FrameIndex() const
    {
        return m_frameIndex;
    }

    ComPtr<ID3D12Resource> GetResource(int n)
    {
        ComPtr<ID3D12Resource> resource;
        ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&resource)));
        return resource;
    }
};
