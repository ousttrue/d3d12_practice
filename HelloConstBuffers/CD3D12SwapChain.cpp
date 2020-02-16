#include "CD3D12SwapChain.h"
#include "d3dhelper.h"

CD3D12SwapChain::CD3D12SwapChain()
{
}

void CD3D12SwapChain::Create(
    const ComPtr<IDXGIFactory4> &factory,
    const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width, int height)
{
    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = BACKBUFFER_COUNT;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        commandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain));
    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    m_swapChain->GetDesc1(&swapChainDesc);
    width = swapChainDesc.Width;
    height = swapChainDesc.Height;
    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
}

void CD3D12SwapChain::Initialize(
    const ComPtr<IDXGIFactory4> &factory,
    const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd)
{
    Create(factory, commandQueue, hwnd, 0, 0);
}

void CD3D12SwapChain::Resize(const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width, int height)
{
    ComPtr<IDXGIFactory4> factory;
    m_swapChain->GetParent(IID_PPV_ARGS(&factory));

    ////////////////////
    // release !
    ////////////////////
    for (auto &rt : m_renderTargets)
    {
        rt.Release();
    }
    m_swapChain.Reset();

    Create(factory, commandQueue, hwnd, width, height);
}

void CD3D12SwapChain::Prepare(const ComPtr<ID3D12Device> &device)
{
    if (!m_rtvHeap)
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = BACKBUFFER_COUNT,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
    }
    if (!m_dsvHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = BACKBUFFER_COUNT,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
    }

    // Create frame resources.
    if (!m_renderTargets[0].Buffer)
    {
        auto rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        auto rtvIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        auto dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        auto dsvIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        // Create a RTV for each frame.
        for (UINT n = 0; n < BACKBUFFER_COUNT;
             n++,
                  rtvHandle.ptr += rtvIncrement,
                  dsvHandle.ptr += dsvIncrement)
        {
            auto rt = &m_renderTargets[n];
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&rt->Buffer)));
            device->CreateRenderTargetView(rt->Buffer.Get(), nullptr, rtvHandle);
            rt->RTV = rtvHandle;
            ;

            // Create depth
            auto depthDesc = rt->Buffer->GetDesc();
            depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
            depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                            D3D12_HEAP_FLAG_NONE,
                                            &depthDesc,
                                            D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                            &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
                                            IID_PPV_ARGS(&rt->DepthStencil));
            device->CreateDepthStencilView(rt->DepthStencil.Get(), nullptr, dsvHandle);
            rt->DSV = dsvHandle;
        }
    }
}

void CD3D12SwapChain::Begin(
    const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor)
{
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    auto rt = &m_renderTargets[m_frameIndex];

    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    D3D12_RESOURCE_BARRIER barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(rt->Buffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
    };
    commandList->ResourceBarrier(_countof(barriers), barriers);
    commandList->OMSetRenderTargets(1, &rt->RTV, FALSE, &rt->DSV);

    commandList->ClearRenderTargetView(rt->RTV, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(rt->DSV, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
}

void CD3D12SwapChain::End(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    auto rt = &m_renderTargets[m_frameIndex];
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rt->Buffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void CD3D12SwapChain::Present()
{
    ThrowIfFailed(m_swapChain->Present(1, 0));
}
