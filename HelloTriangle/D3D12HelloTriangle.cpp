#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers.
#endif

#include <string>
const std::string g_shaders =
#include "shaders.hlsl"
    ;

#include "D3D12HelloTriangle.h"
#include "util.h"
#include "Scene.h"
#include "CommandBuilder.h"
#include <windows.h>
#include <DirectXMath.h>

class Impl
{
    Fence m_fence;
    Swapchain m_swapchain;
    CommandBuilder m_builder;

    bool m_useWarpDevice = false;

    // Viewport dimensions.
    float m_aspectRatio = 1.0f;

    // // Pipeline objects.
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};

    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;

public:
    Impl(bool useWarpDevice, UINT frameCount)
        : m_useWarpDevice(useWarpDevice), m_swapchain(frameCount)
    {
        m_viewport.MinDepth = D3D12_MIN_DEPTH;
        m_viewport.MaxDepth = D3D12_MAX_DEPTH;
    }

    ~Impl()
    {
        // Ensure that the GPU is no longer referencing resources that are about to be
        // cleaned up by the destructor.
        m_fence.Wait(m_commandQueue);
    }

    void SetSize(int w, int h)
    {
        if (w == m_viewport.Width && h == m_viewport.Height)
        {
            auto a = 0;
            return;
        }
        m_viewport.Width = static_cast<float>(w);
        m_viewport.Height = static_cast<float>(h);
        m_aspectRatio = static_cast<float>(w) / static_cast<float>(h);

        m_scissorRect.right = static_cast<LONG>(w);
        m_scissorRect.bottom = static_cast<LONG>(h);
    }

    void Render(HWND hWnd)
    {
        if (m_viewport.Width == 0 || m_viewport.Height == 0)
        {
            return;
        }

        if (!m_device)
        {
            Initialize(hWnd);
            m_builder.Initialize(m_device, m_aspectRatio);
            // Create synchronization objects and wait until assets have been uploaded to the GPU.
            m_fence.Initialize(m_device);
        }

        // Record all the commands we need to render the scene into the command list.
        auto commandList = m_builder.PopulateCommandList(
            m_viewport, m_scissorRect,
            m_swapchain.CurrentRTV(), m_swapchain.CurrentHandle());

        // Execute the command list.
        m_commandQueue->ExecuteCommandLists(1, commandList.GetAddressOf());

        // Present the frame.
        m_swapchain.Present();
        m_fence.Wait(m_commandQueue);
        m_swapchain.UpdateFrameIndex();
    }

private:
    void Initialize(HWND hWnd)
    {
        auto factory = CreateFactory();

        if (m_useWarpDevice)
        {
            ComPtr<IDXGIAdapter> warpAdapter;
            ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

            ThrowIfFailed(D3D12CreateDevice(
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_device)));
        }
        else
        {
            auto hardwareAdapter = GetHardwareAdapter(factory);

            ThrowIfFailed(D3D12CreateDevice(
                hardwareAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_device)));
        }

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        };

        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

        m_swapchain.Initialize(factory, m_commandQueue,
                               hWnd, (UINT)m_viewport.Width, (UINT)m_viewport.Height);
        m_swapchain.CreateRenderTargets(m_device);
    }
};

D3D12HelloTriangle::D3D12HelloTriangle(bool useWarpDevice, UINT frameCount)
    : m_impl(new Impl(useWarpDevice, frameCount))
{
}

D3D12HelloTriangle::~D3D12HelloTriangle()
{
    delete m_impl;
}

void D3D12HelloTriangle::Render(void *hWnd)
{
    m_impl->Render((HWND)hWnd);
}

void D3D12HelloTriangle::SetSize(int w, int h)
{
    m_impl->SetSize(w, h);
}
