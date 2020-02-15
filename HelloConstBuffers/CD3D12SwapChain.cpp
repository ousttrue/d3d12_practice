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
    for (int i = 0; i < BACKBUFFER_COUNT; ++i)
    {
        m_renderTargets[i].Reset();
    }
    m_swapChain.Reset();

    Create(factory, commandQueue, hwnd, width, height);
}

void CD3D12SwapChain::Prepare(const ComPtr<ID3D12Device> &device)
{
    if (!m_rtvHeap)
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = BACKBUFFER_COUNT;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
        m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
    // Create frame resources.
    if (!m_renderTargets[0])
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < BACKBUFFER_COUNT; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }
}

void CD3D12SwapChain::Begin(
    const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor)
{
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void CD3D12SwapChain::End(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void CD3D12SwapChain::Present()
{
    ThrowIfFailed(m_swapChain->Present(1, 0));
}
