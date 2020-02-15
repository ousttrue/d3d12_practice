#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "d3dx12.h"

static const UINT BACKBUFFER_COUNT = 2;

class CD3D12SwapChain
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<IDXGISwapChain3> m_swapChain;
    UINT m_frameIndex = 0;
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<ID3D12Resource> m_renderTargets[BACKBUFFER_COUNT];
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    UINT m_rtvDescriptorSize = 0;

public:
    CD3D12SwapChain();
    void Create(
        const ComPtr<IDXGIFactory4> &factory,
        const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width, int height);
    void Initialize(
        const ComPtr<IDXGIFactory4> &factory,
        const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd);
    void Resize(const ComPtr<ID3D12CommandQueue> &commandQueue, HWND hwnd, int width, int height);

    void Prepare(const ComPtr<ID3D12Device> &device);
    void Begin(const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor);
    void End(const ComPtr<ID3D12GraphicsCommandList> &commandList);
    void Present();
};
