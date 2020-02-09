#include "Swapchain.h"

void Swapchain::Initialize(const ComPtr<IDXGIFactory4> &factory,
                           const ComPtr<ID3D12CommandQueue> &queue,
                           HWND hWnd, UINT w, UINT h)
{
    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        .Width = w,
        .Height = h,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = m_frameCount,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        // .Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT,
    };

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        queue.Get(), // Swap chain needs the queue so that it can force a flush on it.
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // m_swapChain->SetMaximumFrameLatency(1);
}

void Swapchain::CreateRenderTargets(const ComPtr<ID3D12Device> &device)
{
    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = m_frameCount,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < m_frameCount; n++)
        {
            auto resource = GetResource(n);
            device->CreateRenderTargetView(resource.Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
            m_renderTargets.push_back(resource);
        }
    }
}

void Swapchain::Present()
{
    ThrowIfFailed(m_swapChain->Present(1, 0));
    UpdateFrameIndex();
}
