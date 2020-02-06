#pragma once
#include <string>
#include <stdexcept>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }

private:
    const HRESULT m_hr;
};

static void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

static Microsoft::WRL::ComPtr<IDXGIFactory4> CreateFactory()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    return factory;
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
static Microsoft::WRL::ComPtr<IDXGIAdapter1> GetHardwareAdapter(const Microsoft::WRL::ComPtr<IDXGIFactory2> &pFactory)
{
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            return adapter;
        }
    }

    return nullptr;
}

class Fence
{
    HANDLE m_fenceEvent = NULL;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 1;

public:
    Fence()
    {
    }
    ~Fence()
    {
        CloseHandle(m_fenceEvent);
    }

    void Initialize(const ComPtr<ID3D12Device> &device)
    {
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    void Wait(const ComPtr<ID3D12CommandQueue> queue)
    {
        // Signal and increment the fence value.
        const UINT64 fence = m_fenceValue++;
        ThrowIfFailed(queue->Signal(m_fence.Get(), fence));

        // Wait until the previous frame is finished.
        if (m_fence->GetCompletedValue() < fence)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }
};

class Swapchain
{
    ComPtr<IDXGISwapChain3> m_swapChain;
    UINT m_frameIndex = 0;

public:
    Swapchain()
    {
    }
    ~Swapchain()
    {
    }

    void Initialize(const ComPtr<IDXGIFactory4> &factory,
                    const ComPtr<ID3D12CommandQueue> &queue,
                    UINT frameCount,
                    HWND hWnd, UINT width, UINT height)
    {
        // Describe and create the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = width,
            .Height = height,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = {
                .Count = 1,
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = frameCount,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
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
    }

    void Present()
    {
        ThrowIfFailed(m_swapChain->Present(1, 0));
    }

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
